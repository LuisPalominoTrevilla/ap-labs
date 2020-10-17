// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 254.
//!+

// Chat is a server that lets clients chat with each other.
package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
)

//!+broadcaster
type client struct {
	name    string
	channel chan<- string // an outgoing message channel
}

type message struct {
	message string
	from    string
	to      string
}

var (
	entering = make(chan client)
	leaving  = make(chan client)
	messages = make(chan message) // all incoming client messages
)

func broadcaster() {
	clients := make(map[string]chan<- string) // all connected clients
	for {
		select {
		case msg := <-messages:
			if msg.to != "" {
				ch := clients[msg.to]
				if ch == nil {
					// TODO: Send feedback to user
					break
				}
				ch <- msg.message
				break
			}

			for name := range clients {
				if msg.from != name {
					clients[name] <- msg.message
				}
			}

		case cli := <-entering:
			clients[cli.name] = cli.channel

		case cli := <-leaving:
			delete(clients, cli.name)
			close(cli.channel)
		}
	}
}

//!-broadcaster

//!+helpers
func sendMessage(msg, from, to string) {
	prompt := from + " > "
	if from == "" {
		prompt = "irc-server > "
	}
	messages <- message{message: prompt + msg, from: from, to: to}
}

//!-helpers

//!+handleConn
func handleConn(conn net.Conn) {
	ch := make(chan string) // outgoing client messages
	go clientWriter(conn, ch)

	who := conn.RemoteAddr().String()
	sendMessage(who+" has arrived", "", "")
	entering <- client{name: who, channel: ch}
	sendMessage("Welcome to the Simple IRC Server", "", who)
	sendMessage("Your user ["+who+"] is successfully logged", "", who)

	input := bufio.NewScanner(conn)
	for input.Scan() {
		sendMessage(input.Text(), who, "")
	}
	// NOTE: ignoring potential errors from input.Err()

	leaving <- client{name: who, channel: ch}
	sendMessage(who+" has left", "", "")
	conn.Close()
}

func clientWriter(conn net.Conn, ch <-chan string) {
	for msg := range ch {
		fmt.Fprintln(conn, msg) // NOTE: ignoring network errors
	}
}

//!-handleConn

//!+main
func main() {
	listener, err := net.Listen("tcp", "localhost:8000")
	if err != nil {
		log.Fatal(err)
	}

	go broadcaster()
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Print(err)
			continue
		}
		go handleConn(conn)
	}
}

//!-main
