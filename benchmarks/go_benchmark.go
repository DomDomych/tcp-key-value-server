package main

import (
	"bufio"
	"flag"
	"fmt"
	"math/rand"
	"net"
	"sync"
	"time"
	"math"
	"sort"
)

type ClientStats struct {
	Get 	int
	Set 	int
	Del 	int
	Successful 	int 
	Latencies []time.Duration
}

type ClientResult struct {
	ClientID int
	Stats ClientStats
	Err error
}

func percentile(latencies []time.Duration, p float64) time.Duration {
	if len(latencies) == 0 {
		return 0
	}

	index := int(math.Ceil(p*float64(len(latencies)))) - 1

	return latencies[index]
}

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
) (ClientStats,error) {

	stats := ClientStats{}
	conn, err := net.Dial("tcp", address)

	if err != nil {
		return stats,err
	}

	defer conn.Close()

	reader := bufio.NewReader(conn)

	rng := rand.New(rand.NewSource(seed))

	for i := 0; i < requests; i++ {
		operation := rng.Intn(100)

		keyID := rng.Intn(keyspace)
		key := fmt.Sprintf("key_%d", keyID)
		var command string

		if operation < getPercent {
			command = fmt.Sprintf("GET %s\n", key)
		} else if operation < getPercent+setPercent {
			command = fmt.Sprintf("SET %s value\n", key)
		} else {
			command = fmt.Sprintf("DEL %s\n", key)
		}

		requestStart := time.Now()

		_, err = sendCommand(conn, reader, command)

		latency := time.Since(requestStart)

		if err != nil {
			return stats,err
		}

		stats.Latencies = append(stats.Latencies,latency)


		if operation < getPercent{
			stats.Get++
		} else if operation < getPercent+setPercent{
			stats.Set++
		} else {
			stats.Del++
		}

		stats.Successful++
	}


	return stats,nil
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

	start := time.Now()

	var wg sync.WaitGroup

	results := make(chan ClientResult,*clients)

	wg.Add(*clients)

	for i:=0;i<*clients;i++{
		go func(clientID int){
			defer wg.Done()

			stats,err := runClient(
				address,
				*requests,
				*getPercent,
				*setPercent,
				*delPercent,
				*keyspace,
				*seed+int64(clientID),
			)


				results <- ClientResult {
					ClientID: clientID,
					Stats : stats,
					Err : err,
				}
		}(i)
	}

	wg.Wait()

	elapsed := time.Since(start)

	

	total := ClientStats{}

	for i:=0; i<*clients;i++ {
		result := <-results

		if result.Err != nil{
			fmt.Println("Client",result.ClientID,"error:",result.Err)
			
		}

		total.Get+=result.Stats.Get
		total.Set+=result.Stats.Set
		total.Del+=result.Stats.Del
		total.Successful+=result.Stats.Successful

		total.Latencies = append(total.Latencies,result.Stats.Latencies...)

	}

	sort.Slice(total.Latencies, func(i, j int) bool {
		return total.Latencies[i] < total.Latencies[j]
	})

	p50 := percentile(total.Latencies, 0.50)
	p95 := percentile(total.Latencies, 0.95)
	p99 := percentile(total.Latencies, 0.99)
	var latencySum time.Duration

	for _, latency := range total.Latencies {
		latencySum += latency
	}

	var avgLatency time.Duration

	if len(total.Latencies) > 0 {
		avgLatency = latencySum / time.Duration(len(total.Latencies))
	}

	throughput := float64(total.Successful) / elapsed.Seconds()

	expected := *clients * *requests
	fmt.Println()
	fmt.Println("===== RESULT =====")
	fmt.Println("Clients:", *clients)
	fmt.Println("Expected:", expected)
	fmt.Println("Successful:", total.Successful)

	fmt.Println()
	fmt.Println("Operations:")
	fmt.Println("GET:", total.Get)
	fmt.Println("SET:", total.Set)
	fmt.Println("DEL:", total.Del)
	fmt.Printf("Time: %.6f seconds\n", elapsed.Seconds())
	fmt.Printf("Throughput: %.2f commands/sec\n", throughput)
	fmt.Printf(
		"Latency avg: %.3f ms\n",
		float64(avgLatency.Microseconds())/1000.0,
	)

	fmt.Printf(
		"Latency p50: %.3f ms\n",
		float64(p50.Microseconds())/1000.0,
	)

	fmt.Printf(
		"Latency p95: %.3f ms\n",
		float64(p95.Microseconds())/1000.0,
	)

	fmt.Printf(
		"Latency p99: %.3f ms\n",
		float64(p99.Microseconds())/1000.0,
)

}
