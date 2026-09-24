package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"math"
	"os"
	"path/filepath"
)

const canvasSize = 512

type RGBAColor struct {
	R, G, B, A float64
}

func lerp(a, b, t float64) float64 {
	return a + (b-a)*t
}

func clamp(v, min, max float64) float64 {
	if v < min {
		return min
	}
	if v > max {
		return max
	}
	return v
}

// renderIconPixel computes the anti-aliased RGBA color of the GeoDark icon at normalized coordinate (u, v) in [-1, 1]
func renderIconPixel(u, v float64) color.RGBA {
	dist := math.Hypot(u, v)

	// Outer squircle / circle background
	// Radius = 0.90
	outerRadius := 0.90
	if dist > outerRadius+0.02 {
		return color.RGBA{0, 0, 0, 0}
	}

	outerAlpha := clamp((outerRadius-dist)/0.02, 0, 1)
	if outerAlpha <= 0 {
		return color.RGBA{0, 0, 0, 0}
	}

	// Base background: Split Day (left) and Night (right)
	// Left side: Sunrise golden gradient (#ff9500 to #ffcc00, warm sky #ffecd2)
	// Right side: Midnight indigo (#0d1117 to #161b22, deep blue #1e293b)
	var bgR, bgG, bgB float64

	// Split angle: vertical divider with slight solar diagonal tilt (-12 degrees)
	tiltAngle := -15.0 * math.Pi / 180.0
	rotU := u*math.Cos(tiltAngle) - v*math.Sin(tiltAngle)
	rotV := u*math.Sin(tiltAngle) + v*math.Cos(tiltAngle)

	dividerX := rotU
	dividerWidth := 0.035
	splitFactor := clamp((dividerX+dividerWidth/2)/dividerWidth, 0, 1)

	// Left: Warm sunrise daylight gradient
	dayTop := RGBAColor{R: 255, G: 160, B: 50, A: 1.0}
	dayBottom := RGBAColor{R: 255, G: 215, B: 100, A: 1.0}
	dayT := (v + 1.0) / 2.0
	dayR := lerp(dayTop.R, dayBottom.R, dayT)
	dayG := lerp(dayTop.G, dayBottom.G, dayT)
	dayB := lerp(dayTop.B, dayBottom.B, dayT)

	// Right: Deep midnight navy / dark slate gradient
	nightTop := RGBAColor{R: 18, G: 24, B: 38, A: 1.0}
	nightBottom := RGBAColor{R: 35, G: 45, B: 70, A: 1.0}
	nightT := (v + 1.0) / 2.0
	nightR := lerp(nightTop.R, nightBottom.R, nightT)
	nightG := lerp(nightTop.G, nightBottom.G, nightT)
	nightB := lerp(nightTop.B, nightBottom.B, nightT)

	bgR = lerp(dayR, nightR, splitFactor)
	bgG = lerp(dayG, nightG, splitFactor)
	bgB = lerp(dayB, nightB, splitFactor)

	// Subtle outer rim border / glow
	borderDist := math.Abs(dist - outerRadius)
	if borderDist < 0.04 {
		glow := clamp(1.0-borderDist/0.04, 0, 1)
		bgR = lerp(bgR, 255, glow*0.35)
		bgG = lerp(bgG, 255, glow*0.35)
		bgB = lerp(bgB, 255, glow*0.35)
	}

	// Foreground celestial elements:
	// Sun on Left side (center around rotU = -0.32, rotV = -0.05, radius = 0.26)
	sunCenterU := -0.30
	sunCenterV := -0.02
	sunDist := math.Hypot(rotU-sunCenterU, rotV-sunCenterV)
	sunRadius := 0.24

	if sunDist < sunRadius+0.02 {
		sunAlpha := clamp((sunRadius-sunDist)/0.02, 0, 1)
		// Sun body: radiant white-yellow
		sunR := 255.0
		sunG := 250.0
		sunB := 240.0
		bgR = lerp(bgR, sunR, sunAlpha*0.95)
		bgG = lerp(bgG, sunG, sunAlpha*0.95)
		bgB = lerp(bgB, sunB, sunAlpha*0.95)
	}

	// Sun rays on left edge
	if rotU < 0 {
		angle := math.Atan2(rotV-sunCenterV, rotU-sunCenterU)
		rayCount := 8.0
		rayWave := math.Cos(angle * rayCount)
		if rayWave > 0.6 && sunDist > sunRadius+0.05 && sunDist < sunRadius+0.16 {
			rayAlpha := (rayWave - 0.6) / 0.4 * clamp((0.16-(sunDist-sunRadius))/0.11, 0, 1) * 0.75
			bgR = lerp(bgR, 255, rayAlpha)
			bgG = lerp(bgG, 245, rayAlpha)
			bgB = lerp(bgB, 200, rayAlpha)
		}
	}

	// Moon on Right side: Crescent moon centered around rotU = 0.32, rotV = -0.02
	moonCenterU := 0.30
	moonCenterV := -0.02
	moonDist1 := math.Hypot(rotU-moonCenterU, rotV-moonCenterV)
	// Cutout circle slightly shifted to create crescent
	cutoutU := moonCenterU - 0.12
	cutoutV := moonCenterV - 0.07
	moonDist2 := math.Hypot(rotU-cutoutU, rotV-cutoutV)
	moonRadius1 := 0.25
	moonRadius2 := 0.23

	inMoon1 := clamp((moonRadius1-moonDist1)/0.02, 0, 1)
	inMoon2 := clamp((moonRadius2-moonDist2)/0.02, 0, 1)
	moonAlpha := inMoon1 * (1.0 - inMoon2)

	if moonAlpha > 0 {
		moonR := 235.0
		moonG := 242.0
		moonB := 255.0
		bgR = lerp(bgR, moonR, moonAlpha*0.95)
		bgG = lerp(bgG, moonG, moonAlpha*0.95)
		bgB = lerp(bgB, moonB, moonAlpha*0.95)
	}

	// Stars on right side
	stars := [][3]float64{
		{0.55, -0.45, 0.022},
		{0.68, -0.15, 0.018},
		{0.52, 0.35, 0.020},
		{0.32, 0.50, 0.016},
		{0.70, 0.25, 0.014},
	}
	for _, star := range stars {
		sd := math.Hypot(rotU-star[0], rotV-star[1])
		if sd < star[2]+0.01 {
			sAlpha := clamp((star[2]-sd)/0.01, 0, 1)
			bgR = lerp(bgR, 255, sAlpha*0.9)
			bgG = lerp(bgG, 255, sAlpha*0.9)
			bgB = lerp(bgB, 255, sAlpha*0.9)
		}
	}

	// Globe / Geo Meridian Arcs along equator and longitude
	// Meridian arc: ellipse in lower half (rotV = 0.35)
	meridianY := rotV - 0.25
	meridianDist := math.Abs(meridianY*meridianY*3.0 + rotU*rotU - 0.45)
	if meridianDist < 0.025 && dist < outerRadius-0.03 {
		mAlpha := clamp(1.0-meridianDist/0.025, 0, 1) * 0.35
		bgR = lerp(bgR, 255, mAlpha)
		bgG = lerp(bgG, 255, mAlpha)
		bgB = lerp(bgB, 255, mAlpha)
	}

	// Location pin / center nexus at the division point (rotU = 0, rotV = 0.25)
	pinU := 0.0
	pinV := 0.25
	pinDist := math.Hypot(rotU-pinU, rotV-pinV)
	pinRadius := 0.075
	if pinDist < pinRadius+0.015 {
		pinAlpha := clamp((pinRadius-pinDist)/0.015, 0, 1)
		// Location pinpoint: clean accent cyan / blue (#007aff)
		bgR = lerp(bgR, 0, pinAlpha*0.9)
		bgG = lerp(bgG, 122, pinAlpha*0.9)
		bgB = lerp(bgB, 255, pinAlpha*0.9)

		// Inner white dot
		if pinDist < 0.035 {
			innerAlpha := clamp((0.035-pinDist)/0.01, 0, 1)
			bgR = lerp(bgR, 255, innerAlpha)
			bgG = lerp(bgG, 255, innerAlpha)
			bgB = lerp(bgB, 255, innerAlpha)
		}
	}

	return color.RGBA{
		R: uint8(clamp(bgR, 0, 255)),
		G: uint8(clamp(bgG, 0, 255)),
		B: uint8(clamp(bgB, 0, 255)),
		A: uint8(clamp(outerAlpha*255, 0, 255)),
	}
}

// renderIcon generates an image of given size with 2x2 supersampling for smooth antialiasing
func renderIcon(size int) *image.RGBA {
	img := image.NewRGBA(image.Rect(0, 0, size, size))
	samples := 2
	step := 1.0 / float64(samples)

	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			var rSum, gSum, bSum, aSum float64

			for sy := 0; sy < samples; sy++ {
				for sx := 0; sx < samples; sx++ {
					px := (float64(x) + (float64(sx)+0.5)*step) / float64(size)
					py := (float64(y) + (float64(sy)+0.5)*step) / float64(size)

					// map [0, 1] to [-1, 1]
					u := px*2.0 - 1.0
					v := py*2.0 - 1.0

					c := renderIconPixel(u, v)
					a := float64(c.A) / 255.0
					rSum += float64(c.R) * a
					gSum += float64(c.G) * a
					bSum += float64(c.B) * a
					aSum += a
				}
			}

			totalSamples := float64(samples * samples)
			finalA := aSum / totalSamples
			if finalA > 0.001 {
				img.SetRGBA(x, y, color.RGBA{
					R: uint8(clamp(rSum/aSum, 0, 255)),
					G: uint8(clamp(gSum/aSum, 0, 255)),
					B: uint8(clamp(bSum/aSum, 0, 255)),
					A: uint8(clamp(finalA*255.0, 0, 255)),
				})
			} else {
				img.SetRGBA(x, y, color.RGBA{0, 0, 0, 0})
			}
		}
	}
	return img
}

func writePNG(path string, img image.Image) error {
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return err
	}
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()
	return png.Encode(f, img)
}

func writeICO(path string, sizes []int) error {
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return err
	}

	images := make([][]byte, len(sizes))
	for i, sz := range sizes {
		var buf bytes.Buffer
		img := renderIcon(sz)
		if err := png.Encode(&buf, img); err != nil {
			return err
		}
		images[i] = buf.Bytes()
	}

	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	// ICONDIR header
	if err := binary.Write(f, binary.LittleEndian, uint16(0)); err != nil {
		return err
	}
	if err := binary.Write(f, binary.LittleEndian, uint16(1)); err != nil { // 1 = ICO
		return err
	}
	if err := binary.Write(f, binary.LittleEndian, uint16(len(images))); err != nil {
		return err
	}

	// ICONDIRENTRY entries
	offset := uint32(6 + 16*len(images))
	for i, data := range images {
		sz := sizes[i]
		w, h := byte(sz), byte(sz)
		if sz == 256 {
			w, h = 0, 0
		}
		if _, err := f.Write([]byte{w, h, 0, 0}); err != nil {
			return err
		}
		if err := binary.Write(f, binary.LittleEndian, uint16(1)); err != nil { // color planes
			return err
		}
		if err := binary.Write(f, binary.LittleEndian, uint16(32)); err != nil { // bits per pixel
			return err
		}
		if err := binary.Write(f, binary.LittleEndian, uint32(len(data))); err != nil {
			return err
		}
		if err := binary.Write(f, binary.LittleEndian, offset); err != nil {
			return err
		}
		offset += uint32(len(data))
	}

	// Image bytes
	for _, data := range images {
		if _, err := f.Write(data); err != nil {
			return err
		}
	}

	return nil
}

func main() {
	fmt.Println("Rendering GeoDark icon...")
	if err := writePNG("assets/icon.png", renderIcon(canvasSize)); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing PNG: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Wrote assets/icon.png (512x512)")

	icoSizes := []int{16, 20, 24, 32, 40, 48, 64, 128, 256}
	if err := writeICO("assets/geodark.ico", icoSizes); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing ICO: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Wrote assets/geodark.ico (multi-resolution 16..256)")
}
