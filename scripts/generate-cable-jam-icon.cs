#:package SkiaSharp@3.119.4
#:package SkiaSharp.NativeAssets.Win32@3.119.4
#:package Svg.Skia@5.0.0

using System.Runtime.CompilerServices;
using System.Text;
using SkiaSharp;
using Svg.Skia;

const string IconFileName = "cable-jam.ico";
const string IconBaseName = "cable-jam";
const int SvgIconSize = 128;

var iconSizes = new[] { 16, 20, 24, 32, 64, 128, 256 };
string scriptPath = ThisFile();
string root = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(scriptPath)!, ".."));
string iconsPath = Path.Combine(root, "icons");
string outputPath = Path.Combine(iconsPath, IconFileName);
Directory.CreateDirectory(iconsPath);

string svgIconText = BuildSvgIcon();
WriteAllTextIfChanged(Path.Combine(iconsPath, $"{IconBaseName}.svg"), svgIconText);

var frames = new List<IconFrame>(iconSizes.Length);
foreach (int size in iconSizes)
{
    using SKData pngData = size <= 24
        ? RenderPixelFrame(size)
        : RenderSvgFrame(size, svgIconText);

    byte[] pngBytes = pngData.ToArray();
    string pngPath = Path.Combine(iconsPath, $"{IconBaseName}-{size}.png");
    WriteAllBytesIfChanged(pngPath, pngBytes);

    frames.Add(new IconFrame(size, pngBytes));
    Console.WriteLine($"Rendered {size}x{size} PNG frame ({pngData.Size} bytes): {pngPath}");
}

WriteIco(outputPath, frames);
Console.WriteLine(outputPath);

static SKData RenderPixelFrame(int size)
{
    var imageInfo = new SKImageInfo(size, size, SKColorType.Bgra8888, SKAlphaType.Premul);
    using var surface = SKSurface.Create(imageInfo);
    SKCanvas canvas = surface.Canvas;
    canvas.Clear(SKColors.Transparent);

    using var bgPaint = PixelPaint(new SKColor(10, 15, 18));
    using var panelPaint = PixelPaint(new SKColor(23, 33, 38));
    using var gridPaint = PixelPaint(new SKColor(39, 54, 60));
    using var borderPaint = PixelPaint(new SKColor(73, 129, 143));
    using var cyanPaint = PixelPaint(new SKColor(48, 199, 209));
    using var pinkPaint = PixelPaint(new SKColor(244, 57, 128));
    using var amberPaint = PixelPaint(new SKColor(247, 179, 61));
    using var endPaint = PixelPaint(new SKColor(245, 241, 216));

    FillRect(canvas, 0, 0, size - 1, size - 1, bgPaint);
    DrawPixelBoard(canvas, size, panelPaint, gridPaint, borderPaint);

    switch (size)
    {
        case 16:
            DrawPixelLine(canvas, new[] { new PixelPoint(3, 4), new PixelPoint(10, 4), new PixelPoint(10, 6) }, cyanPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(4, 8), new PixelPoint(12, 8), new PixelPoint(12, 9) }, pinkPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(3, 13), new PixelPoint(8, 13), new PixelPoint(8, 11), new PixelPoint(13, 11) }, amberPaint);
            DrawPixelEnds(canvas, endPaint, new PixelPoint(3, 4), new PixelPoint(10, 6), new PixelPoint(4, 8), new PixelPoint(12, 9), new PixelPoint(3, 13), new PixelPoint(13, 11));
            break;
        case 20:
            DrawPixelLine(canvas, new[] { new PixelPoint(4, 5), new PixelPoint(13, 5), new PixelPoint(13, 8) }, cyanPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(5, 10), new PixelPoint(16, 10), new PixelPoint(16, 13) }, pinkPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(4, 17), new PixelPoint(10, 17), new PixelPoint(10, 14), new PixelPoint(16, 14) }, amberPaint);
            DrawPixelEnds(canvas, endPaint, new PixelPoint(4, 5), new PixelPoint(13, 8), new PixelPoint(5, 10), new PixelPoint(16, 13), new PixelPoint(4, 17), new PixelPoint(16, 14));
            break;
        case 24:
            DrawPixelLine(canvas, new[] { new PixelPoint(5, 6), new PixelPoint(16, 6), new PixelPoint(16, 9) }, cyanPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(6, 12), new PixelPoint(20, 12), new PixelPoint(20, 15) }, pinkPaint);
            DrawPixelLine(canvas, new[] { new PixelPoint(5, 20), new PixelPoint(13, 20), new PixelPoint(13, 17), new PixelPoint(20, 17) }, amberPaint);
            DrawPixelEnds(canvas, endPaint, new PixelPoint(5, 6), new PixelPoint(16, 9), new PixelPoint(6, 12), new PixelPoint(20, 15), new PixelPoint(5, 20), new PixelPoint(20, 17));
            break;
        default:
            throw new ArgumentOutOfRangeException(nameof(size), size, "No pixel frame is defined for this icon size.");
    }

    using SKImage image = surface.Snapshot();
    return image.Encode(SKEncodedImageFormat.Png, 100);
}

static SKData RenderSvgFrame(int size, string svgIconText)
{
    using SKSvg svg = LoadSvg(svgIconText);

    var imageInfo = new SKImageInfo(size, size, SKColorType.Bgra8888, SKAlphaType.Premul);
    using var surface = SKSurface.Create(imageInfo);
    SKCanvas canvas = surface.Canvas;
    canvas.Clear(SKColors.Transparent);
    DrawSvgToFrame(canvas, svg, size, size);

    using SKImage image = surface.Snapshot();
    return image.Encode(SKEncodedImageFormat.Png, 100);
}

static string BuildSvgIcon()
{
    return $$"""
        <svg xmlns="http://www.w3.org/2000/svg" width="{{SvgIconSize}}" height="{{SvgIconSize}}" viewBox="0 0 {{SvgIconSize}} {{SvgIconSize}}">
          <rect x="6" y="6" width="116" height="116" rx="18" fill="#0a0f12"/>
          <rect x="16" y="16" width="96" height="96" rx="8" fill="#172126" stroke="#4a8290" stroke-width="3"/>
          <path d="M 40 16 L 40 112 M 64 16 L 64 112 M 88 16 L 88 112 M 16 40 L 112 40 M 16 64 L 112 64 M 16 88 L 112 88" stroke="#2b3d43" stroke-width="1.5"/>

          <path d="M 27 34 L 80 34 L 80 49" fill="none" stroke="#30c7d1" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>
          <path d="M 31 63 L 96 63 L 96 79" fill="none" stroke="#f43980" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>
          <path d="M 27 96 L 57 96 L 57 82 L 97 82" fill="none" stroke="#f7b33d" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>

          <g fill="#f5f1d8">
            <circle cx="27" cy="34" r="4"/>
            <circle cx="80" cy="49" r="4"/>
            <circle cx="31" cy="63" r="4"/>
            <circle cx="96" cy="79" r="4"/>
            <circle cx="27" cy="96" r="4"/>
            <circle cx="97" cy="82" r="4"/>
          </g>
        </svg>
        """;
}

static void DrawPixelBoard(SKCanvas canvas, int size, SKPaint panelPaint, SKPaint gridPaint, SKPaint borderPaint)
{
    int left = Math.Max(1, size / 10);
    int top = Math.Max(1, size / 10);
    int right = size - left - 1;
    int bottom = size - top - 1;

    FillRect(canvas, left, top, right, bottom, panelPaint);
    FillRect(canvas, left, top, right, top, borderPaint);
    FillRect(canvas, left, bottom, right, bottom, borderPaint);
    FillRect(canvas, left, top, left, bottom, borderPaint);
    FillRect(canvas, right, top, right, bottom, borderPaint);

    int step = size switch
    {
        16 => 3,
        20 => 4,
        24 => 5,
        _ => 4,
    };

    for (int x = left + step; x < right; x += step)
    {
        FillRect(canvas, x, top + 1, x, bottom - 1, gridPaint);
    }

    for (int y = top + step; y < bottom; y += step)
    {
        FillRect(canvas, left + 1, y, right - 1, y, gridPaint);
    }
}

static void DrawPixelLine(SKCanvas canvas, IReadOnlyList<PixelPoint> points, SKPaint paint)
{
    for (int index = 0; index < points.Count - 1; index++)
    {
        PixelPoint a = points[index];
        PixelPoint b = points[index + 1];

        FillRect(canvas, Math.Min(a.X, b.X), Math.Min(a.Y, b.Y), Math.Max(a.X, b.X), Math.Max(a.Y, b.Y), paint);
    }
}

static void DrawPixelEnds(SKCanvas canvas, SKPaint paint, params PixelPoint[] points)
{
    foreach (PixelPoint point in points)
    {
        FillRect(canvas, point.X, point.Y, point.X, point.Y, paint);
    }
}

static SKPaint PixelPaint(SKColor color)
{
    return new SKPaint
    {
        Color = color,
        IsAntialias = false,
        Style = SKPaintStyle.Fill,
    };
}

static void FillRect(SKCanvas canvas, int left, int top, int right, int bottom, SKPaint paint)
{
    canvas.DrawRect(SKRectI.Create(left, top, right - left + 1, bottom - top + 1), paint);
}

static SKSvg LoadSvg(string svgText)
{
    var svg = new SKSvg();
    using var stream = new MemoryStream(Encoding.UTF8.GetBytes(svgText));
    svg.Load(stream);

    if (svg.Picture is null)
    {
        svg.Dispose();
        throw new InvalidOperationException("Failed to load icon SVG.");
    }

    return svg;
}

static void DrawSvgToFrame(SKCanvas canvas, SKSvg svg, int width, int height)
{
    SKPicture picture = svg.Picture ?? throw new InvalidOperationException("SVG has no picture.");
    SKRect bounds = picture.CullRect;
    float scale = MathF.Min(width / bounds.Width, height / bounds.Height);
    float dx = (width - bounds.Width * scale) / 2.0f - bounds.Left * scale;
    float dy = (height - bounds.Height * scale) / 2.0f - bounds.Top * scale;

    canvas.Save();
    canvas.Translate(dx, dy);
    canvas.Scale(scale);
    canvas.DrawPicture(picture);
    canvas.Restore();
}

static void WriteAllBytesIfChanged(string path, byte[] bytes)
{
    if (File.Exists(path))
    {
        byte[] existingBytes = File.ReadAllBytes(path);
        if (existingBytes.AsSpan().SequenceEqual(bytes))
        {
            return;
        }
    }

    File.WriteAllBytes(path, bytes);
}

static void WriteAllTextIfChanged(string path, string text)
{
    if (File.Exists(path) && File.ReadAllText(path, Encoding.UTF8) == text)
    {
        return;
    }

    File.WriteAllText(path, text, Encoding.UTF8);
}

static void WriteIco(string path, IReadOnlyList<IconFrame> frames)
{
    using FileStream stream = File.Open(path, FileMode.Create, FileAccess.Write, FileShare.None);
    using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: false);

    writer.Write((ushort)0);
    writer.Write((ushort)1);
    writer.Write((ushort)frames.Count);

    uint imageOffset = 6u + (uint)frames.Count * 16u;
    foreach (IconFrame frame in frames)
    {
        writer.Write((byte)(frame.Size == 256 ? 0 : frame.Size));
        writer.Write((byte)(frame.Size == 256 ? 0 : frame.Size));
        writer.Write((byte)0);
        writer.Write((byte)0);
        writer.Write((ushort)1);
        writer.Write((ushort)32);
        writer.Write((uint)frame.PngBytes.Length);
        writer.Write(imageOffset);
        imageOffset += (uint)frame.PngBytes.Length;
    }

    foreach (IconFrame frame in frames)
    {
        writer.Write(frame.PngBytes);
    }
}

static string ThisFile([CallerFilePath] string path = "") => path;

readonly record struct IconFrame(int Size, byte[] PngBytes);

readonly record struct PixelPoint(int X, int Y);
