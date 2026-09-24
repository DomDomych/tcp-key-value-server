package main

import (
	"bufio"
	"flag"
	"fmt"
	"math/rand"
	"net"
	"sync"
)

func sendCommand(conn net.Conn, reader *bufio.Reader, command string) (string, error) {
	_, err := conn.Write([]byte(command))

	if err != nil {
		return "", err
	}

	response, err := reader.ReadString('\n')

	if err != nil {
		return "", err
	}

	return response, nil
}

func populateKeyspace(address string, keyspace int) error {
	conn, err := net.Dial("tcp", address)

	if err != nil {
		return err
	}

	defer conn.Close()

	reader := bufio.NewReader(conn)

	for i := 0; i < keyspace; i++ {
		command := fmt.Sprintf(
			"SET key_%d value_%d\n",
			i,
			i,
		)

		response, err := sendCommand(conn, reader, command)

		if err != nil {
			return err
		}

		if response != "OK\n" {
			return fmt.Errorf("unexpected server response: %s", response)
		}
	}

	return nil
}

func runClient(
	address string,
	requests int,
	getPercent int,
	setPercent int,
	delPercent int,
	keyspace int,
	seed int64,
) error {
	conn, err := net.Dial("tcp", address)

	if err != nil {
		return err
	}

	defer conn.Close()

	reader := bufio.NewReader(conn)

	rng := rand.New(rand.NewSource(seed))

	getCount := 0
	setCount := 0
	delCount := 0
	for i := 0; i < requests; i++ {
		operation := rng.Intn(100)

		keyID := rng.Intn(keyspace)
		key := fmt.Sprintf("key_%d", keyID)
		var command string

		if operation < getPercent {
			command = fmt.Sprintf("GET %s\n", key)
			getCount++
		} else if operation < getPercent+setPercent {
			command = fmt.Sprintf("SET %s value\n", key)
			setCount++
		} else {
			command = fmt.Sprintf("DEL %s\n", key)
			delCount++
		}

		_, err = sendCommand(conn, reader, command)

		if err != nil {
			return err
		}
	}

	fmt.Println("GET:", getCount)
	fmt.Println("SET:", setCount)
	fmt.Println("DEL:", delCount)

	return nil
}

func main() {
	host := flag.String("host", "127.0.0.1", "server host")
	port := flag.Int("port", 8080, "server port")

	clients := flag.Int("clients", 1, "number of concurrent clients")
	requests := flag.Int("requests", 100, "request per client")

	getPercent := flag.Int("get", 80, "GET percentage")
	setPercent := flag.Int("set", 15, "SET percentage")
	delPercent := flag.Int("del", 5, "DEL percentage")

	keyspace := flag.Int("keyspace", 1000, "nubmer of keys")
	seed := flag.Int64("seed", 42, "random seed")

	flag.Parse()

	fmt.Println("Host:", *host)
	fmt.Println("Port:", *port)
	fmt.Println("Clients:", *clients)
	fmt.Println("Requests/client:", *requests)
	fmt.Println("GET:", *getPercent)
	fmt.Println("SET:", *setPercent)
	fmt.Println("DEL:", *delPercent)
	fmt.Println("Keyspace:", *keyspace)
	fmt.Println("Seed:", *seed)

	if *clients <= 0 {
		fmt.Println("clients must be greater than 0")
		return
	}

	if *requests <= 0 {
		fmt.Println("request must be greater than 0")
		return
	}

	if *keyspace <= 0 {
		fmt.Println("keyspace must be greater than 0")
		return
	}

	if *getPercent+*setPercent+*delPercent != 100 {
		fmt.Println("GET + SET + DEL must equal 100")
		return
	}

	address := fmt.Sprintf("%s:%d", *host, *port)

	fmt.Println("Populating keyspace...")

	err := populateKeyspace(address, *keyspace)

	if err != nil {
		fmt.Println("Populating error:", err)
		return
	}

	fmt.Println("Keyspace populated")

	fmt.Println("Server:", address)

	fmt.Println("Starting clients...")

	var wg sync.WaitGroup

	wg.Add(*clients)

	for i:=0;i<*clients;i++{
		go func(clientID int){
			defer wg.Done()

			err := runClient(
				address,
				*requests,
				*getPercent,
				*setPercent,
				*delPercent,
				*keyspace,
				*seed+int64(clientID),
			)

			if err!=nil{
				fmt.Println("Client",clientID,"error:",err)
			}
		}(i)
	}

	wg.Wait()

	fmt.Println("All clients finished succesfully")

}
