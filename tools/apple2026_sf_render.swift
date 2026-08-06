// Render an SF Symbol straight from the system, no asset pack needed.
// usage: swift sfrender.swift <symbol.name> <pointsize> <out.png>
import AppKit

let a = CommandLine.arguments
guard a.count >= 4, let pt = Double(a[2]) else { exit(2) }
let name = a[1]

guard let img = NSImage(systemSymbolName: name, accessibilityDescription: nil) else {
    FileHandle.standardError.write("MISSING \(name)\n".data(using: .utf8)!)
    exit(3)
}
let cfg = NSImage.SymbolConfiguration(pointSize: CGFloat(pt), weight: .regular)
guard let sized = img.withSymbolConfiguration(cfg) else { exit(4) }

let r = NSRect(origin: .zero, size: sized.size)
guard let cg = sized.cgImage(forProposedRect: nil, context: nil, hints: nil) else { exit(5) }
let rep = NSBitmapImageRep(cgImage: cg)
rep.size = r.size
guard let png = rep.representation(using: .png, properties: [:]) else { exit(6) }
try png.write(to: URL(fileURLWithPath: a[3]))
print("OK \(name) \(Int(r.width))x\(Int(r.height))")
