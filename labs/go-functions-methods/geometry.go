// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 156.

// Package geometry defines simple types for plane geometry.
//!+point
package main

import (
	"errors"
	"fmt"
	"math"
	"math/rand"
	"os"
	"strconv"
	"time"
)

// Point represented by x and y coordinates
type Point struct{ x, y float64 }

// X returns the point's y coordinate
func (p Point) X() float64 {
	return p.x
}

// Y returns the point's y coordinate
func (p Point) Y() float64 {
	return p.y
}

// Distance traditional function
func Distance(p, q Point) float64 {
	return math.Hypot(q.X()-p.X(), q.Y()-p.Y())
}

// Distance same thing, but as a method of the Point type
func (p Point) Distance(q Point) float64 {
	return math.Hypot(q.X()-p.X(), q.Y()-p.Y())
}

//!-point

//!+path

// A Path is a journey connecting the points with straight lines.
type Path []Point

// Distance returns the distance traveled along the path.
func (path Path) Distance() []float64 {
	n := len(path)
	distance := make([]float64, 0, n)
	for i := range path {
		distance = append(distance, path[i].Distance(path[(i+1)%n]))
	}
	return distance
}

//!-path

func max(x, y float64) float64 {
	if x < y {
		return y
	}
	return x
}

func min(x, y float64) float64 {
	if x < y {
		return x
	}
	return y
}

// Removes index i from path s
func remove(s Path, i int) Path {
	s[i] = s[len(s)-1]
	return s[:len(s)-1]
}

func onSegment(p, q, r Point) bool {
	return (q.X() <= max(p.X(), r.X())) && (q.X() >= min(p.X(), r.X())) &&
		(q.Y() <= max(p.Y(), r.Y())) && (q.Y() >= min(p.Y(), r.Y()))
}

func orientation(p, q, r Point) uint8 {
	val := ((q.Y() - p.Y()) * (r.X() - q.X())) - ((q.X() - p.X()) * (r.Y() - q.Y()))
	if val > 0 {
		// Clockwise orientation
		return 1
	} else if val < 0 {
		// Counterclockwise orientation
		return 2
	} else {
		// Colinear orientation
		return 0
	}
}

func doIntersect(p1, q1, p2, q2 Point) bool {
	o1 := orientation(p1, q1, p2)
	o2 := orientation(p1, q1, q2)
	o3 := orientation(p2, q2, p1)
	o4 := orientation(p2, q2, q1)

	// general case
	if o1 != o2 && o3 != o4 {
		return true
	}

	// Special cases
	if o1 == 0 && onSegment(p1, p2, q1) {
		return true
	}
	if o2 == 0 && onSegment(p1, q2, q1) {
		return true
	}
	if o3 == 0 && onSegment(p2, p1, q2) {
		return true
	}
	if o4 == 0 && onSegment(p2, q1, q2) {
		return true
	}

	return false
}

// Validates whether line segment p2p3 is valid with adjacent p1p2 segment
func isAdjacentValid(p1, p2, p3 Point) bool {
	if orientation(p1, p2, p3) != 0 {
		return true
	}

	return !onSegment(p1, p3, p2) && !onSegment(p3, p1, p2)
}

// Validates whether new point complies with current segments (i.e. does not intersect)
func doSegmentsComply(figure Path, newPoint Point) bool {
	n := len(figure)
	i := 0
	if n < 2 {
		return true
	}

	if figure[0] == newPoint {
		i = 1
		if !isAdjacentValid(figure[1], figure[0], figure[n-1]) {
			return false
		}
	}

	if !isAdjacentValid(figure[n-2], figure[n-1], newPoint) {
		return false
	}

	for ; i < n-2; i++ {
		if doIntersect(figure[i], figure[i+1], figure[n-1], newPoint) {
			return false
		}
	}

	return true
}

func validPolygon(figure, points Path, newPoint Point) Path {
	if !doSegmentsComply(figure, newPoint) {
		return nil
	}

	updatedFig := make(Path, len(figure), cap(figure))
	copy(updatedFig, figure)
	updatedFig = append(updatedFig, newPoint)

	if len(points) == 0 {
		if !doSegmentsComply(updatedFig, updatedFig[0]) {
			return nil
		}

		return updatedFig
	}

	for i, point := range points {
		updatedPoints := make(Path, len(points), cap(points))
		copy(updatedPoints, points)
		updatedPoints = remove(updatedPoints, i)
		res := validPolygon(updatedFig, updatedPoints, point)
		if res != nil {
			return res
		}
	}

	return nil
}

func generateFigure(sides int) (Path, error) {
	if sides < 3 {
		return nil, errors.New("At least 3 sides are needed to create figure")
	}

	rand.Seed(time.Now().UnixNano())
	points := make(Path, 0, sides)
	var figure Path

	for figure == nil {
		for i := 0; i < sides; i++ {
			x := math.Round((rand.Float64()*200-100)*100) / 100
			y := math.Round((rand.Float64()*200-100)*100) / 100
			points = append(points, Point{x, y})
		}

		figure = validPolygon(Path{}, points[1:], points[0])
	}

	return figure, nil
}

func main() {
	var err error
	var sides int
	var figure Path

	if len(os.Args) != 2 {
		println("Usage: ./geometry {sides}")
		return
	}

	sides, err = strconv.Atoi(os.Args[1])
	if err != nil {
		println("Error:", err.Error())
		return
	}

	figure, err = generateFigure(sides)
	if err != nil {
		println("Error:", err.Error())
		return
	}

	fmt.Printf("- Generating a [%d] sides figure\n", sides)
	fmt.Println("- Figure's vertices")
	for _, v := range figure {
		fmt.Printf("  - (%.2f, %.2f)\n", v.X(), v.Y())
	}

	perim := 0.0
	fmt.Printf("- Figure's perimeter\n  - ")
	for i, d := range figure.Distance() {
		perim += d
		sep := " + "
		if i == len(figure)-1 {
			sep = " = "
		}
		fmt.Printf("%.2f%s", d, sep)
	}

	fmt.Printf("%.2f\n", perim)
}
