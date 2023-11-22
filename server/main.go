package main

import (
	"crypto/tls"
	"log"
	"net"
	"unsafe"

	imgui "github.com/AllenDang/cimgui-go"
)

type client struct {
	show bool
	addr string
	log  string
	cmd  string
}

var cl []client

var globalCmd string
var cmdLog string
var showDemo bool

func loop() {
	imgui.DockSpaceOverViewportV(imgui.MainViewport(), imgui.DockNodeFlagsPassthruCentralNode, imgui.NewWindowClass())

	if showDemo {
		imgui.PlotShowDemoWindowV(&showDemo)
	}

	imgui.SetNextWindowSizeV(imgui.NewVec2(300, 300), imgui.CondOnce)
	imgui.BeginV("PCs", nil, 0)

	imgui.Checkbox("show demo", &showDemo)

	for i := range cl {
		imgui.Checkbox(cl[i].addr, &cl[i].show)
	}

	imgui.Separator()

	if imgui.InputTextWithHint("##globalInput", "global cmd", &globalCmd, imgui.InputTextFlagsEnterReturnsTrue|imgui.InputTextFlagsEscapeClearsAll, nil) {
		if globalCmd != "" {
			globalCmd = ""
			//for range cl { push to chan }
		}
	}

	imgui.End()

	for i := 0; i < len(cl); i++ {
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
							//push to channel instead
							cl[i].log += cl[i].cmd + "\n"
							cl[i].cmd = ""
						}
					}

					imgui.PopItemWidth()
					imgui.EndTabItem()
				}

				if imgui.BeginTabItem("misc") {
					imgui.EndTabItem()
				}

				imgui.EndTabBar()
			}

			imgui.End()
		}
	}

	//fmt.Println(imgui.CurrentIO().Framerate())
}

func tlsAccept(sv net.Listener) {
	defer sv.Close()
	for {
		conn, err := sv.Accept()
		if err != nil {
			log.Println(err)
			continue
		}

		conn.Write([]byte("hello world\n"))
		conn.Close()
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

	cl = []client{{addr: "127.0.0.1:54"}, {addr: "654654654"}, {addr: "555555"}}

	backend.Run(loop)
}

func afterCreateContextHook() {
	imgui.PlotCreateContext()
}

func beforeDestroyContextHook() {
	imgui.PlotDestroyContext()
}
