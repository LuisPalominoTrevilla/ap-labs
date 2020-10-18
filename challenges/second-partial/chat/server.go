// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 254.
//!+

// Chat is a server that lets clients chat with each other.
package main

import (
	"bufio"
	"bytes"
	"flag"
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
	isAdmin bool
	conn    net.Conn
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
	kick     = make(chan string)
	clients  map[string]*client
)

func broadcaster() {
	clients = make(map[string]*client) // all connected clients
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

		case name := <-kick:
			if cli, exists := clients[name]; exists {
				delete(clients, name)
				close(cli.channel)
				cli.conn.Close()
			}

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

func logInfo(msg string) {
	fmt.Printf("irc-server > %s\n", msg)
}

func parseCommand(cmd string, cli *client) {
	words := strings.Split(cmd, " ")
	if len(words) == 0 {
		return
	}

	switch words[0] {
	case "/users":
		for _, user := range clients {
			sendMessage(user.name+" - connected since "+user.created.Format("2006-01-02 15:04:05"), "", cli.name)
		}
	case "/msg":
		if len(words) < 3 {
			sendMessage("Command usage: /msg <user> <msg>", "", cli.name)
			break
		}
		sendMessage(strings.Join(words[2:], " "), cli.name, words[1])
	case "/time":
		now := time.Now()
		tz, _ := now.Zone()
		sendMessage("Local Time: "+tz+" "+now.Format("15:04"), "", cli.name)
	case "/user":
		if len(words) < 2 {
			sendMessage("Command usage: /user <user>", "", cli.name)
			break
		}
		user, exists := clients[words[1]]
		if !exists {
			sendMessage("No user named "+words[1]+" found", "", cli.name)
			break
		}
		sendMessage("username: "+user.name+", IP: "+user.ip+", connected since: "+user.created.Format("2006-01-02 15:04:05"), "", cli.name)
	case "/kick":
		if !cli.isAdmin {
			sendMessage("Authorization required", "", cli.name)
			break
		}
		if len(words) < 2 {
			sendMessage("Command usage: /user <user>", "", cli.name)
			break
		}
		user, exists := clients[words[1]]
		if !exists {
			sendMessage("No user named "+words[1]+" found", "", cli.name)
			break
		}
		if user == cli {
			sendMessage("You can't kick yourself out", "", cli.name)
			break
		}
		sendMessage("You're kicked from this channel", "", user.name)
		sendMessage("Bad language is not allowed on this channel", "", user.name)
		kick <- user.name
		sendMessage("["+user.name+"] was kicked from channel for bad language policy violation", "", "")
		logInfo("[" + user.name + "]" + " was kicked")
	default:
		if cmd != "" {
			sendMessage(cmd, cli.name, "")
		}
	}
}

//!-helpers

//!+handleConn
func handleConn(conn net.Conn) {
	tmp := make([]byte, 16)
	conn.Read(tmp)
	tmp = bytes.Trim(tmp, "\x00")

	ch := make(chan string) // outgoing client messages

	cli := new(client)
	cli.name = string(tmp)
	cli.channel = ch
	cli.ip = conn.RemoteAddr().String()
	cli.created = time.Now()
	cli.isAdmin = len(clients) == 0
	cli.conn = conn

	go clientWriter(conn, ch)

	if _, exists := clients[cli.name]; exists {
		ch <- "irc-server > Another user already exists with the same username"
		close(ch)
		conn.Close()
		return
	}

	logInfo("New connected user [" + cli.name + "]")
	sendMessage("New connected user ["+cli.name+"]", "", "")
	entering <- cli
	sendMessage("Welcome to the Simple IRC Server", "", cli.name)
	sendMessage("Your user ["+cli.name+"] is successfully logged", "", cli.name)
	if cli.isAdmin {
		logInfo("[" + cli.name + "] was promoted as the channel ADMIN")
		sendMessage("Congrats, you were the first user", "", cli.name)
		sendMessage("You're the new IRC Server ADMIN", "", cli.name)
	}

	input := bufio.NewScanner(conn)
	for input.Scan() {
		parseCommand(input.Text(), cli)
	}
	if err := input.Err(); err != nil {
		return
	}

	leaving <- cli.name
	logInfo("[" + cli.name + "]" + " left")
	sendMessage("["+cli.name+"]"+" left channel", "", "")
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
	host := flag.String("host", "localhost", "server host")
	port := flag.String("port", "9000", "server port")
	flag.Parse()
	address := *host + ":" + *port
	listener, err := net.Listen("tcp", address)
	if err != nil {
		log.Fatal(err)
	}
	logInfo("Simple IRC Server started at " + address)
	logInfo("Ready for receiving new clients")

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
