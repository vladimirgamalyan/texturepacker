#include <filesystem>
#include <sstream>
#include "external/cli11/CLI11.hpp"
#include "external/maxRectsBinPack/MaxRectsBinPack.h"
#include "external/pugixml/pugixml.hpp"
#include "ImgLoaderExt.h"
#include "ImgWriter.h"

static char asciitolower(char in) 
{
    if (in <= 'Z' && in >= 'A')
        return in - ('Z' - 'z');
    return in;
}

static std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), asciitolower);
    return s;
}

static std::string dirnameOf(const std::string& fname)
{
    size_t pos = fname.find_last_of("\\/");
    return (std::string::npos == pos) ? std::string() : fname.substr(0, pos);
}

struct ParentPathValidator : public CLI::Validator
{
    ParentPathValidator()
    {
        name_ = "PATH(parentexisting)";
        func_ = [](const std::string& p)
        {
            if (!std::filesystem::is_directory(dirnameOf(p)))
                return "Parent path does not exist: " + p;
            if (std::filesystem::is_directory(p))
                return "Path is actually a directory: " + p;
            return std::string();
        };
    }
};

const static ParentPathValidator ParentPath;

struct Size
{
    Size() = default;
    Size(std::uint32_t w, std::uint32_t h) : w(w), h(h) {}
    std::uint32_t w = 0;
    std::uint32_t h = 0;
};

int main(int argc, char* argv[])
{
    CLI::App app{ "Texture Packer" };

    std::string src;
    std::string dst;
    app.add_option("source", src, "source directory")->required()->check(CLI::ExistingDirectory);
    app.add_option("output", dst, "output files")->required()->check(ParentPath);

    CLI11_PARSE(app, argc, argv);

    try
    {
        std::vector<std::string> sources;
        for (const auto& p : std::filesystem::directory_iterator(src))
        {
            const std::string ext = to_lower(p.path().extension().string());
            if (ext == ".png")
                sources.push_back(p.path().string());
        }

        const int spacingHor = 1;
        const int spacingVer = 1;
        const bool cropTexturesWidth = true;
        const bool cropTexturesHeight = true;

        std::vector<rbp::RectSize> rects;
        std::vector<ImgLoaderExt> images(sources.size());
        for (size_t i = 0; i < sources.size(); ++i)
        {
            if (!images[i].load(sources[i]))
                throw std::runtime_error("error load file " + sources[i]);
            rects.emplace_back(images[i].cropRect.w + spacingHor, images[i].cropRect.h + spacingVer, i);
        }
    
        std::vector<Size> textureSizeList = {
            {64, 64},
            {128, 64},
            {128, 128},
            {256, 128},
            {256, 256},
            {512, 256},
            {512, 512},
            {1024, 512},
            {1024, 1024},
            {2048, 1024},
            {2048, 2048},
        };
        std::vector<Size> result;
        rbp::MaxRectsBinPack mrbp;

        for (;;)
        {
            std::vector<rbp::Rect> arrangedRectangles;
            auto glyphRectanglesCopy = rects;
            Size lastSize;

            uint64_t allGlyphSquare = 0;
            for (const auto& i : rects)
                allGlyphSquare += static_cast<uint64_t>(i.width) * i.height;

            for (size_t i = 0; i < textureSizeList.size(); ++i)
            {
                const auto& ss = textureSizeList[i];
                uint64_t textureSquare = static_cast<uint64_t>(ss.w) * ss.h;
                if (textureSquare < allGlyphSquare && i + 1 < textureSizeList.size())
                    continue;

                lastSize = ss;
                rects = glyphRectanglesCopy;

                const auto workAreaW = ss.w - spacingHor;
                const auto workAreaH = ss.h - spacingVer;

                mrbp.Init(workAreaW, workAreaH);
                mrbp.Insert(rects, arrangedRectangles, rbp::MaxRectsBinPack::RectBestAreaFit);

                if (rects.empty())
                    break;
            }

            if (arrangedRectangles.empty())
            {
                if (!rects.empty())
                    throw std::runtime_error("can not fit glyphs into texture");
                break;
            }

            std::uint32_t maxX = 0;
            std::uint32_t maxY = 0;
            for (const auto& r : arrangedRectangles)
            {
                std::uint32_t x = r.x + spacingHor;
                std::uint32_t y = r.y + spacingVer;

                const int w = images[r.tag].cropRect.w + spacingHor;
                const int h = images[r.tag].cropRect.h + spacingVer;

                images[r.tag].flipped = w != r.width;
                images[r.tag].x = x;
                images[r.tag].y = y;
                images[r.tag].page = static_cast<int>(result.size());

                assert(images[r.tag].flipped ? w == r.height && h == r.width : w == r.width && h == r.height);

                if (maxX < x + r.width)
                    maxX = x + r.width;
                if (maxY < y + r.height)
                    maxY = y + r.height;
            }
            if (cropTexturesWidth)
                lastSize.w = maxX;
            if (cropTexturesHeight)
                lastSize.h = maxY;

            result.push_back(lastSize);
        }

        std::stringstream ss;
        for (size_t p = 0; p < result.size(); ++p)
        {
            pugi::xml_document doc;
            auto decl = doc.append_child(pugi::node_declaration);
            decl.append_attribute("version") = "1.0";
            decl.append_attribute("encoding") = "UTF-8";
            doc.append_child(pugi::node_doctype).set_value("plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");

            auto plist = doc.append_child("plist");
            plist.append_attribute("version").set_value("1.0");
            auto rootDict = plist.append_child("dict");
            rootDict.append_child("key").text().set("frames");
            auto frames = rootDict.append_child("dict");

            ImgWriter imgWriter(result[p].w, result[p].h);
    #ifdef _DEBUG
            imgWriter.fill(0xff0000ff);
    #endif
            for (const auto& i : images)
            {
                assert(i.page != -1);
                if (i.page != p)
                    continue;

                imgWriter.paste(i);

                const std::string f = std::filesystem::relative(i.lastFileName, src).generic_string();
                frames.append_child("key").text().set(f.c_str());
                auto frame = frames.append_child("dict");
                frame.append_child("key").text().set("frame");
                ss.str(std::string());
                ss << "{{" << i.x << "," << i.y << "},{" << i.cropRect.w << "," << i.cropRect.h << "}}";
                frame.append_child("string").text().set(ss.str().c_str());
                frame.append_child("key").text().set("offset");
                ss.str(std::string());
                double offsetX = i.cropRect.w / 2.0 + i.cropRect.x - i.getW() / 2.0;
                double offsetY = i.cropRect.h / 2.0 + i.cropRect.y - i.getH() / 2.0;
                double intpart;
                if (std::modf(offsetX, &intpart) != 0.0 || std::modf(offsetY, &intpart) != 0.0)
                    throw std::runtime_error("invalid offset");
                ss << "{" << offsetX << "," << -offsetY << "}";
                frame.append_child("string").text().set(ss.str().c_str());
                frame.append_child("key").text().set("rotated");
                frame.append_child(i.flipped ? "true" : "false");
                frame.append_child("key").text().set("sourceColorRect");
                ss.str(std::string());
                ss << "{{" << i.cropRect.x << "," << i.cropRect.y << "},{" << i.cropRect.w << "," << i.cropRect.h << "}}";
                frame.append_child("string").text().set(ss.str().c_str());
                frame.append_child("key").text().set("sourceSize");
                ss.str(std::string());
                ss << "{" << i.getW() << "," << i.getH() << "}";
                frame.append_child("string").text().set(ss.str().c_str());
            }

            imgWriter.write(dst + "_" + std::to_string(p)  + ".png");


            rootDict.append_child("key").text().set("metadata");
            auto metadata = rootDict.append_child("dict");
            metadata.append_child("key").text().set("format");
            metadata.append_child("integer").text().set("2");
            metadata.append_child("key").text().set("pixelFormat");
            metadata.append_child("string").text().set("RGBA8888");
            metadata.append_child("key").text().set("premultiplyAlpha");
            metadata.append_child("false");
            metadata.append_child("key").text().set("realTextureFileName");
            std::string realTextureFileName = std::filesystem::path(dst + "_" + std::to_string(p) + ".png").filename().generic_string();
            metadata.append_child("string").text().set(realTextureFileName.c_str());
            metadata.append_child("key").text().set("size");
            ss.str(std::string());
            ss << "{" << result[p].w << "," << result[p].h << "}";
            metadata.append_child("string").text().set(ss.str().c_str());
            metadata.append_child("key").text().set("textureFileName");
            metadata.append_child("string").text().set(realTextureFileName.c_str());

            if (!doc.save_file((dst + "_" + std::to_string(p) + ".plist").c_str()))
                throw::std::runtime_error("error save xml document");
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << "\n";
    }
}
