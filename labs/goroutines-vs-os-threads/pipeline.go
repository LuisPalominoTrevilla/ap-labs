package main

import (
	"fmt"
	"time"
)

// If program never consumes all memory, modify max number to test
// a fixed amount of pipeline stages
const maxAllowedStages = 10000000

func main() {
	var maxStages int
	ch := make(chan struct{})

	defer func() {
		start := time.Now()
		for i := maxStages; i > 0; i-- {
			ch <- struct{}{}
		}
		transitTime := time.Now().Sub(start).Seconds()
		fmt.Println("Maximum number of pipeline stages   : ", maxStages)
		fmt.Println("Time to transit trough the pipeline : ", transitTime, "seconds")
	}()

	for maxStages = 0; maxStages < maxAllowedStages; maxStages++ {
		go func() { <-ch }()
	}
}
