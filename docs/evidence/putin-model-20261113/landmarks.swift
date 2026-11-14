import Foundation
import Vision
import ImageIO

let url = URL(fileURLWithPath: CommandLine.arguments[1])
guard let source = CGImageSourceCreateWithURL(url as CFURL, nil), let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else { fatalError("Image unavailable") }
let req = VNDetectFaceLandmarksRequest()
try VNImageRequestHandler(cgImage: image, options: [:]).perform([req])
guard let f = req.results?.first, let l = f.landmarks else { fatalError("No face detected") }
let fields: [(String,VNFaceLandmarkRegion2D?)] = [("contour",l.faceContour),("leftEye",l.leftEye),("rightEye",l.rightEye),("leftBrow",l.leftEyebrow),("rightBrow",l.rightEyebrow),("nose",l.nose),("noseCrest",l.noseCrest),("outerLips",l.outerLips)]
var result = [String: [[Double]]]()
for (name, region) in fields {
    guard let r=region else { continue }
    result[name] = r.normalizedPoints.map { p in
        [Double((f.boundingBox.origin.x + p.x * f.boundingBox.width) * CGFloat(image.width)), Double((1 - f.boundingBox.origin.y - p.y * f.boundingBox.height) * CGFloat(image.height))]
    }
}
let data=try JSONSerialization.data(withJSONObject: result,options:[.sortedKeys])
print(String(decoding:data,as:UTF8.self))
