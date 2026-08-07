// Pulsador para el simulador: toma el foco, manda la ráfaga y devuelve el
// foco a la app que lo tenía.
// Uso: swift keypress.swift <código>[:<ms>] [<código>[:<ms>] ...]
import Cocoa

guard let sim = NSWorkspace.shared.runningApplications.first(where: { $0.localizedName == "rockboxui" }) else {
    print("rockboxui no está corriendo"); exit(1)
}
let prev = NSWorkspace.shared.frontmostApplication
sim.activate(options: [.activateIgnoringOtherApps])
usleep(400_000)
for spec in CommandLine.arguments.dropFirst() {
    let parts = spec.split(separator: ":")
    let code = CGKeyCode(UInt16(parts[0])!)
    let ms = parts.count > 1 ? UInt32(parts[1])! : 150
    CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: true)?.post(tap: .cghidEventTap)
    usleep(ms * 1000)
    CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: false)?.post(tap: .cghidEventTap)
    usleep(120_000)
}
usleep(200_000)
prev?.activate(options: [.activateIgnoringOtherApps])
// (ampliación en burst.swift)
