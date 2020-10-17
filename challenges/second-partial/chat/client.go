// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 227.

// Netcat is a simple read/write client for TCP servers.
package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

var username *string

//!+
func main() {
	username = flag.String("user", "anonymous", "username")
	flag.Parse()
	fmt.Println("Hi ", *username)
	conn, err := net.Dial("tcp", "localhost:8000")
	if err != nil {
		log.Fatal(err)
	}
	conn.Write([]byte(*username))
	done := make(chan struct{})
	go displayMessages(os.Stdout, conn, done)
	go sendMessages(conn, os.Stdin)
	<-done // wait for background goroutine to finish
	conn.Close()
}

//!-

func displayMessages(dst io.Writer, src io.Reader, done chan struct{}) {
	reader := bufio.NewScanner(src)
	for reader.Scan() {
		_, err := fmt.Fprintf(dst, "\r\033[K\r%s\n%s > ", reader.Text(), *username)
		if err != nil {
			log.Fatal(err)
		}
	}
	if err := reader.Err(); err != nil {
		log.Fatal(err)
	}
	log.Println("done")
	done <- struct{}{} // signal the main goroutine
}

func sendMessages(dst io.Writer, src io.Reader) {
	reader := bufio.NewReader(src)
	for {
		fmt.Printf("%s > ", *username)
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
