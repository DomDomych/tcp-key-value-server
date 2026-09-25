package integration

import(
	"bufio"
	"fmt"
	"net"
	"testing"
)

func sendCommand(t *testing.T,conn net.Conn,reader *bufio.Reader,command string)string{
	t.Helper()

	_,err:=fmt.Fprintf(conn,"%s\n",command)

	if err!=nil{
		t.Fatalf("failed to send %q: %v",command,err)
	}

	response,err := reader.ReadString('\n')

	if err!=nil{
		t.Fatalf("failed to read response for %q: %v",command,err)
	}

	return response
}


func TestSetGetDel(t *testing.T) {
	conn, err := net.Dial("tcp", "127.0.0.1:8080")
	if err != nil {
		t.Fatalf("failed to connect to server: %v", err)
	}
	defer conn.Close()

	reader := bufio.NewReader(conn)


	t.Cleanup(func() {
		fmt.Fprintf(conn, "DEL test_key\n")
		reader.ReadString('\n')
	})

	tests := []struct {
		command  string
		expected string
	}{
		{"SET test_key test_value", "OK\n"},
		{"GET test_key", "test_value\n"},
		{"DEL test_key", "OK\n"},
		{"GET test_value", "NO SUCH KEY\n"},
	}

	for _, tt := range tests {
		t.Run(tt.command, func(t *testing.T) {
			got := sendCommand(t, conn, reader, tt.command)

			if got != tt.expected {
				t.Errorf(
					"%s: expected %q, got %q",
					tt.command,
					tt.expected,
					got,
				)
			}
		})
	}
}