// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 227.

// Netcat is a simple read/write client for TCP servers.
package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

//!+
func main() {
	conn, err := net.Dial("tcp", "localhost:8000")
	if err != nil {
		log.Fatal(err)
	}
	done := make(chan struct{})
	go displayMessages(os.Stdout, conn, done)
	go mustCopy(conn, os.Stdin)
	<-done // wait for background goroutine to finish
	conn.Close()
}

//!-

func displayMessages(dst io.Writer, src io.Reader, done chan struct{}) {
	reader := bufio.NewReader(src)
	for {
		message, err := reader.ReadString('\n')
		if err != nil {
			log.Fatal(err)
			break
		}

		_, wErr := fmt.Fprintf(dst, "\r%s$ > ", message)
		if wErr != nil {
			log.Fatal(wErr)
			break
		}
	}
	log.Println("done")
	done <- struct{}{} // signal the main goroutine
}

func mustCopy(dst io.Writer, src io.Reader) {
	reader := bufio.NewReader(src)
	for {
		fmt.Print("$ > ")
		cmdString, err := reader.ReadString('\n')
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			continue
		}

		_, wErr := dst.Write([]byte(cmdString))
		if wErr != nil {
			fmt.Fprintln(os.Stderr, err)
		}
	}
}
