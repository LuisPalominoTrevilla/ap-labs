package main

import "golang.org/x/tour/pic"

func Pic(dx, dy int) [][]uint8 {
	pict := make([][]uint8, dy)
	for y := 0; y < dy; y++ {
		pict[y] = make([]uint8, dx)
		for x := 0; x < dx; x++ {
			pict[y][x] = (uint8)(x^y)
		}
	}
	return pict
}

func main() {
	pic.Show(Pic)
}
