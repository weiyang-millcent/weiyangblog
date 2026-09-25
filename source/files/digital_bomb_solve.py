from PIL import Image, ImageDraw


days = [
    [25, 31, 32, 36, 37, 40, 43, 44, 45, 46, 47, 55, 56, 57, 58, 59, 61, 69, 98, 106, 120, 129, 138, 139, 140, 143, 144],
    [25, 30, 35, 43, 59, 61, 68, 69, 97, 98, 106, 120, 128, 129, 137, 141, 145],
    [0, 1, 3, 7, 8, 9, 13, 14, 15, 19, 20, 21, 24, 25, 26, 29, 30, 31, 35, 39, 40, 43, 44, 45, 46, 58, 61, 62, 63, 64, 69, 74, 75, 76, 77, 86, 87, 88, 89, 91, 93, 94, 98, 103, 104, 105, 106, 115, 116, 117, 120, 121, 122, 123, 127, 129, 132, 134, 135, 141, 145],
    [0, 2, 4, 6, 10, 12, 16, 18, 25, 30, 35, 40, 47, 57, 61, 65, 69, 73, 85, 89, 91, 92, 98, 102, 106, 114, 120, 124, 126, 129, 132, 133, 140, 145],
    [0, 2, 4, 6, 10, 12, 13, 14, 15, 16, 18, 25, 30, 34, 35, 40, 47, 57, 61, 65, 69, 74, 75, 76, 85, 89, 91, 98, 102, 106, 114, 120, 124, 126, 127, 128, 129, 130, 132, 139, 145, 146],
    [0, 2, 4, 6, 10, 12, 18, 25, 30, 35, 40, 43, 47, 56, 61, 65, 69, 77, 86, 87, 88, 89, 91, 98, 102, 106, 114, 120, 124, 129, 132, 145],
    [0, 2, 4, 7, 8, 9, 13, 14, 15, 19, 20, 21, 26, 27, 30, 35, 39, 40, 41, 44, 45, 46, 56, 61, 65, 68, 69, 70, 73, 74, 75, 76, 89, 91, 97, 98, 99, 103, 104, 105, 106, 115, 116, 117, 120, 124, 129, 132, 139, 145],
    [35, 85, 89, 145],
    [36, 37, 49, 50, 51, 52, 53, 79, 80, 81, 82, 83, 86, 87, 88, 108, 109, 110, 111, 112, 143, 144],
]

width = max(max(day) for day in days) + 1
height = len(days)

# Print a terminal preview. Two characters per pixel make the cells look squarer.
for day in days:
    points = set(day)
    print("".join("##" if x in points else "  " for x in range(width)))

# Also save a magnified PNG for easier reading.
scale = 8
margin = 16
image = Image.new(
    "RGB",
    (width * scale + margin * 2, height * scale + margin * 2),
    "white",
)
draw = ImageDraw.Draw(image)

for y, day in enumerate(days):
    for x in day:
        left = margin + x * scale
        top = margin + y * scale
        draw.rectangle(
            (left, top, left + scale - 1, top + scale - 1),
            fill="black",
        )

for x in range(width + 1):
    position = margin + x * scale
    draw.line(
        (position, margin, position, margin + height * scale),
        fill="#dddddd",
    )

for y in range(height + 1):
    position = margin + y * scale
    draw.line(
        (margin, position, margin + width * scale, position),
        fill="#dddddd",
    )

image.save("digital_bomb_result.png")
print(f"\nsize: {width} x {height}")
print("saved: digital_bomb_result.png")
