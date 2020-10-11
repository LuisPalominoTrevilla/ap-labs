package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"strings"
	"sync"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Expected at least one clock server to be specified.")
		os.Exit(1)
	}

	var wg sync.WaitGroup
	for _, arg := range os.Args[1:] {
		clockServer := strings.Split(arg, "=")
		if len(clockServer) < 2 {
			fmt.Printf("Wrong clock server format %s. Expected: <locationName>=<server>\n", arg)
			continue
		}
		server := clockServer[1]

		wg.Add(1)
		go handleClock(server, &wg)
	}

	wg.Wait()
}

func handleClock(server string, wg *sync.WaitGroup) {
	defer wg.Done()
	conn, err := net.Dial("tcp", server)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()
	if _, err := io.Copy(os.Stdout, conn); err != nil {
		log.Fatal(err)
	}
}
