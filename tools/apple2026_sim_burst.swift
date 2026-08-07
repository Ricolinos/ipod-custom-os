// Ráfaga de keydowns repetidos (emula la rueda girando): burst.swift <código> <n> <gap_ms>
import Cocoa
let a = CommandLine.arguments
let code = CGKeyCode(UInt16(a[1])!), n = Int(a[2])!, gap = UInt32(a[3])!
for i in 0..<n {
    let d = CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: true)
    if i > 0 { d?.setIntegerValueField(.keyboardEventAutorepeat, value: 1) }
    d?.post(tap: .cghidEventTap)
    usleep(gap * 1000)
}
CGEvent(keyboardEventSource: nil, virtualKey: code, keyDown: false)?.post(tap: .cghidEventTap)
