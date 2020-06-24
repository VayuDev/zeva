class Image {
    construct new(width, height) {
        if (width >= 2000 || height >= 2000) {
            Fiber.abort("Too much memory requested!")
        }
        _width = width
        _height = height
        _pixels = List.filled(width * height, [0, 1, 1])
    }
    setPixel(x, y, color) {
        _pixels[y * _width + x] = color
    }
    foreign exportInternal(pixels, width, height)
    export() {
        return exportInternal(_pixels, _width, _height)
    }

    foreign static hsvToRgb(h, s, v)
    static hsvToRgb(color) {
        return hsvToRgb(color[0], color[1], color[2])
    }
}
