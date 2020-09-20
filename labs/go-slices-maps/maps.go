package main

import (
	"golang.org/x/tour/wc"
	"strings"
)

func WordCount(s string) map[string]int {
	count := make(map[string]int)
	words := strings.Fields(s)
	for _, word := range words {
		count[word] += 1
	}

	return count
}

func main() {
	wc.Test(WordCount)
}
