package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"time"
)

func handleConn(c net.Conn, tz string) {
	defer c.Close()
	loc, locErr := time.LoadLocation(tz)

	for {
		now := time.Now()
		if locErr == nil {
			now.In(loc)
		}
		timeInfo := fmt.Sprintf("%s\t: %s", tz, now.Format("15:04:05\n"))
		_, err := io.WriteString(c, timeInfo)
		if err != nil {
			return // e.g., client disconnected
		}
		time.Sleep(1 * time.Second)
	}
}

func main() {
	tz := os.Getenv("TZ")
	if len(tz) == 0 {
		tz = "Local"
	}
	if len(os.Args) < 3 || os.Args[1] != "-port" {
		fmt.Println("Usage: [TZ=<timezone>] go run clock2.go -port <port>")
		os.Exit(1)
	}

	listener, err := net.Listen("tcp", "localhost:"+os.Args[2])
	if err != nil {
		log.Fatal(err)
	}
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Print(err) // e.g., connection aborted
			continue
		}
		go handleConn(conn, tz) // handle connections concurrently
	}
}
