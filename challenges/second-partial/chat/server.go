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
	"strings"
	"time"
)

//!+broadcaster
type client struct {
	name    string
	channel chan<- string // an outgoing message channel
	ip      string
	created time.Time
}

type message struct {
	message string
	from    string
	to      string
}

var (
	entering = make(chan *client)
	leaving  = make(chan string)
	messages = make(chan message)
)

func broadcaster() {
	clients := make(map[string]*client) // all connected clients
	for {
		select {
		case msg := <-messages:
			if msg.to != "" {
				if cli, exists := clients[msg.to]; exists {
					cli.channel <- msg.message
				}
				break
			}

			for name := range clients {
				if msg.from != name {
					clients[name].channel <- msg.message
				}
			}

		case cli := <-entering:
			clients[cli.name] = cli

		case name := <-leaving:
			if cli, exists := clients[name]; exists {
				delete(clients, name)
				close(cli.channel)
			}
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

func parseCommand(cmd, cliName string) {
	words := strings.Split(cmd, " ")
	if len(words) == 0 {
		return
	}

	switch words[0] {
	case "/time":
		now := time.Now()
		tz, _ := now.Zone()
		sendMessage("Local Time: "+tz+" "+now.Format("15:04"), "", cliName)
	default:
		sendMessage(cmd, cliName, "")
	}
}

//!-helpers

//!+handleConn
func handleConn(conn net.Conn) {
	tmp := make([]byte, 16)
	conn.Read(tmp)

	ch := make(chan string) // outgoing client messages

	cli := new(client)
	cli.name = string(tmp)
	cli.channel = ch
	cli.ip = conn.RemoteAddr().String()
	cli.created = time.Now()

	go clientWriter(conn, ch)

	sendMessage(cli.name+" has arrived", "", "")
	entering <- cli
	sendMessage("Welcome to the Simple IRC Server", "", cli.name)
	sendMessage("Your user ["+cli.name+"] is successfully logged", "", cli.name)

	input := bufio.NewScanner(conn)
	for input.Scan() {
		parseCommand(input.Text(), cli.name)
	}
	// NOTE: ignoring potential errors from input.Err()

	leaving <- cli.name
	sendMessage(cli.name+" has left", "", "")
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
