package main

import (
	"crypto/tls"
	"log"
	"net"
	"unsafe"

	imgui "github.com/AllenDang/cimgui-go"
)

func loop() {
	imgui.PlotShowDemoWindow()
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

	backend.CreateWindow("IWA server", 1280, 720)

	io := imgui.CurrentIO()
	io.Fonts().AddFontFromFileTTFV("Roboto-Medium.ttf", 16.0, &imgui.FontConfig{}, (*imgui.Wchar)(unsafe.Pointer(&glyphRanges)))
	io.SetBackendFlags(io.BackendFlags() | imgui.BackendFlagsRendererHasVtxOffset)
	backend.Run(loop)
}

func afterCreateContextHook() {
	imgui.PlotCreateContext()
}

func beforeDestroyContextHook() {
	imgui.PlotDestroyContext()
}
