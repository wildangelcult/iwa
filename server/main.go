package main

import (
	"bufio"
	"crypto/tls"
	"encoding/binary"
	"io"
	"log"
	"math"
	"net"
	"unsafe"

	imgui "github.com/AllenDang/cimgui-go"
)

type client struct {
	show       bool
	addr       string
	log        string
	cmd        string
	cpu        float32
	prot       bool
	plotOffset int32
	plotX      []float32
	plotY      []float32
	send       chan []byte
	recvLog    chan []byte
	recvCpu    chan float32
}

var cl []client

var appendClient chan client
var selectedCmd string
var sumDt float32

var showDemo bool

func remove(slice []client, s int) []client {
	return append(slice[:s], slice[s+1:]...)
}

func strToCmdPacket(cmd string) []byte {
	cmd += "\n"
	size := make([]byte, 2)
	binary.BigEndian.PutUint16(size, uint16(len(cmd)))
	return append(append([]byte{0}, size...), []byte(cmd)...)
}

func loop() {
	select {
	case v := <-appendClient:
		cl = append(cl, v)
	default:
	}

	imgui.DockSpaceOverViewportV(imgui.MainViewport(), imgui.DockNodeFlagsPassthruCentralNode, imgui.NewWindowClass())

	if showDemo {
		imgui.PlotShowDemoWindowV(&showDemo)
	}

	imgui.SetNextWindowSizeV(imgui.NewVec2(300, 300), imgui.CondOnce)
	imgui.BeginV("PCs", nil, 0)

	imgui.BeginChildStrV("boxes", imgui.NewVec2(0, -(imgui.CurrentStyle().ItemSpacing().Y+imgui.FrameHeightWithSpacing())), false, imgui.WindowFlagsHorizontalScrollbar)

	imgui.Checkbox("show demo", &showDemo)

	for i := range cl {
		imgui.Checkbox(cl[i].addr, &cl[i].show)
	}

	imgui.EndChild()

	imgui.Separator()

	imgui.PushItemWidth(-1.0)
	if imgui.InputTextWithHint("##selectInput", "send to selected", &selectedCmd, imgui.InputTextFlagsEnterReturnsTrue|imgui.InputTextFlagsEscapeClearsAll, nil) {
		if selectedCmd != "" {
			packet := strToCmdPacket(selectedCmd)
			for i := range cl {
				if cl[i].show {
					cl[i].send <- packet
				}
			}
			selectedCmd = ""
		}
	}

	imgui.PopItemWidth()
	imgui.End()

	for i := 0; i < len(cl); i++ {
		select {
		case l, ok := <-cl[i].recvLog:
			if ok {
				cl[i].log += string(l)
			} else {
				close(cl[i].send)
				cl = remove(cl, i)
				i--
				continue
			}
		default:
		}

		select {
		case c := <-cl[i].recvCpu:
			cl[i].cpu = c
		default:
		}

		sumDt += imgui.CurrentIO().DeltaTime()
		cl[i].plotX[cl[i].plotOffset] = sumDt
		cl[i].plotY[cl[i].plotOffset] = cl[i].cpu

		cl[i].plotOffset = (cl[i].plotOffset + 1) % int32(len(cl[i].plotX))

		if cl[i].show {
			imgui.SetNextWindowSizeV(imgui.NewVec2(300, 300), imgui.CondOnce)
			imgui.BeginV(cl[i].addr, &cl[i].show, 0)

			if imgui.BeginTabBar("##tabbar") {
				if imgui.BeginTabItem("cmd") {
					imgui.PushItemWidth(-1.0)

					imgui.BeginChildStrV("text", imgui.NewVec2(0, -(imgui.CurrentStyle().ItemSpacing().Y+imgui.FrameHeightWithSpacing())), false, imgui.WindowFlagsHorizontalScrollbar)
					imgui.TextUnformatted(cl[i].log)
					if imgui.ScrollY() >= imgui.ScrollMaxY() {
						imgui.SetScrollHereYV(1.0)
					}
					imgui.EndChild()

					imgui.Separator()
					if imgui.InputTextWithHint("##input", "", &cl[i].cmd, imgui.InputTextFlagsEnterReturnsTrue|imgui.InputTextFlagsEscapeClearsAll, nil) {
						if cl[i].cmd != "" {
							cl[i].send <- strToCmdPacket(cl[i].cmd)
							cl[i].cmd = ""
						}
					}

					imgui.PopItemWidth()
					imgui.EndTabItem()
				}

				if imgui.BeginTabItem("misc") {
					if imgui.PlotBeginPlotV("CPU", imgui.NewVec2(-1, -1), 0) {
						imgui.PlotSetupAxesV("", "", imgui.PlotAxisFlagsNoTickLabels, 0)
						imgui.PlotSetupAxisLimitsV(imgui.AxisX1, float64(sumDt-10), float64(sumDt), imgui.CondAlways)
						imgui.PlotSetupAxisLimitsV(imgui.AxisY1, 0, 100, imgui.CondAlways)
						imgui.PlotPlotLineFloatPtrFloatPtrV("##CPU", cl[i].plotX, cl[i].plotY, int32(len(cl[i].plotX)), 0, cl[i].plotOffset, 4)
						imgui.PlotSetNextFillStyleV(imgui.NewVec4(0, 0, 0, -1), 0.5)
						imgui.PlotPlotShadedFloatPtrFloatPtrIntV("##CPU", cl[i].plotX, cl[i].plotY, int32(len(cl[i].plotX)), 0, 0, cl[i].plotOffset, 4)
						imgui.PlotEndPlot()
					}

					imgui.Separator()

					if imgui.Checkbox("Protected?", &cl[i].prot) {
						var b byte
						if cl[i].prot {
							b = 1
						} else {
							b = 0
						}
						cl[i].send <- []byte{2, b}
					}
					imgui.EndTabItem()
				}

				imgui.EndTabBar()
			}

			imgui.End()
		}
	}

	//fmt.Println(imgui.CurrentIO().Framerate())
}

func handleSendConn(conn net.Conn, send chan []byte) {
	for {
		v, ok := <-send
		if ok {
			_, err := conn.Write(v)
			if err != nil {
				return
			}
		} else {
			return
		}
	}
}

func handleRecvConn(conn net.Conn, recvLog chan []byte, recvCpu chan float32) {
	defer conn.Close()
	defer close(recvCpu)
	defer close(recvLog)

	r := bufio.NewReader(conn)
	for {
		var s uint16

		t, err := r.ReadByte()
		if err != nil {
			return
		}

		switch t {
		case 0:
			buf := make([]byte, 0, 4096)

			b, err := r.ReadByte()
			if err != nil {
				return
			}
			s = uint16(b) << 8

			b, err = r.ReadByte()
			if err != nil {
				return
			}
			s |= uint16(b)
			buf = buf[:s]

			_, err = io.ReadFull(r, buf)
			if err != nil {
				return
			}

			recvLog <- buf
		case 1:
			buf := make([]byte, 4)

			_, err = io.ReadFull(r, buf)
			if err != nil {
				return
			}

			recvCpu <- math.Float32frombits(binary.BigEndian.Uint32(buf))
		}
	}
}

func tlsAccept(sv net.Listener) {
	defer sv.Close()
	for {
		conn, err := sv.Accept()
		if err != nil {
			log.Println(err)
			continue
		}

		c := client{
			addr:    conn.RemoteAddr().String(),
			prot:    true,
			plotX:   make([]float32, 2000),
			plotY:   make([]float32, 2000),
			send:    make(chan []byte, 16),
			recvLog: make(chan []byte, 16),
			recvCpu: make(chan float32, 16),
		}

		appendClient <- c
		go handleRecvConn(conn, c.recvLog, c.recvCpu)
		go handleSendConn(conn, c.send)
	}
}

var glyphRanges = [3]imgui.Wchar{0x20, 0xFFFF, 0x0}

func main() {
	cer, err := tls.LoadX509KeyPair("cer/server.crt", "cer/server.key")
	if err != nil {
		log.Println(err)
		return
	}

	sv, err := tls.Listen("tcp", ":1337", &tls.Config{Certificates: []tls.Certificate{cer}})
	if err != nil {
		log.Println(err)
		return
	}
	go tlsAccept(sv)

	//var backend imgui.Backend[imgui.GLFWWindowFlags]
	backend := imgui.CreateBackend(imgui.NewGLFWBackend())
	backend.SetAfterCreateContextHook(afterCreateContextHook)
	backend.SetBeforeDestroyContextHook(beforeDestroyContextHook)

	backend.SetWindowFlags(imgui.GLFWWindowFlagsMaximized, 1)

	backend.CreateWindow("IWA server", 1280, 720)

	io := imgui.CurrentIO()
	io.Fonts().AddFontFromFileTTFV("Roboto-Medium.ttf", 16.0, &imgui.FontConfig{}, (*imgui.Wchar)(unsafe.Pointer(&glyphRanges)))
	io.SetBackendFlags(io.BackendFlags() | imgui.BackendFlagsRendererHasVtxOffset)

	appendClient = make(chan client, 8)

	//cl = []client{{addr: "127.0.0.1:54"}, {addr: "654654654"}, {addr: "555555"}}

	backend.Run(loop)
}

func afterCreateContextHook() {
	imgui.PlotCreateContext()
}

func beforeDestroyContextHook() {
	imgui.PlotDestroyContext()
}
