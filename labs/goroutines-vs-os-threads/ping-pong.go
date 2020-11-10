package main

import (
	"fmt"
	"time"
)

func main() {
	var commsPerSecond int
	ch1 := make(chan int)
	ch2 := make(chan int)
	exit := make(chan int)
	start := time.Now()

	go func(in <-chan int, out chan<- int) {
		for {
			total := <-in
			if time.Now().Sub(start).Seconds() > 1 {
				exit <- total
				return
			}
			out <- total + 1
		}
	}(ch1, ch2)

	ch1 <- 0

	go func(in <-chan int, out chan<- int) {
		for {
			total := <-in
			if time.Now().Sub(start).Seconds() > 1 {
				exit <- total
				return
			}
			out <- total + 1
		}
	}(ch2, ch1)

	commsPerSecond = <-exit

	fmt.Println("Communications Per Second : ", commsPerSecond)
}
