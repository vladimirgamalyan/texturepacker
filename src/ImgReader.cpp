#include "ImgReader.h"
#include <iostream>

#define WUFFS_IMPLEMENTATION
#define WUFFS_CONFIG__STATIC_FUNCTIONS
#define WUFFS_CONFIG__MODULE__PNG
#pragma warning (push)
#pragma warning (disable : 4018)
#pragma warning (disable : 4244)
#pragma warning (disable : 4334)
#include "external/wuffs/wuffs-unsupported-snapshot.c"
#pragma warning (pop)


wuffs_aux::MemOwner g_pixbuf_mem_owner(nullptr, &free);
wuffs_base__pixel_buffer g_pixbuf = { 0 };
 
ImgReader::ImgReader()
{

}
 
bool ImgReader::load(const std::string& fileName)
{
    FILE* f = fopen(fileName.c_str(), "rb");
    if (!f)
    {
        std::cout << "could not open file " << fileName << "\n";
        return false;
    }
    
    g_width = 0;
    g_height = 0;
    g_pixbuf_mem_owner.reset();
    g_pixbuf = wuffs_base__null_pixel_buffer();

    wuffs_aux::DecodeImageCallbacks callbacks;
    wuffs_aux::sync_io::FileInput input(f);
    wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(callbacks, input);
    fclose(f);

    // wuffs_aux::DecodeImageCallbacks's default implementation should give us an
    // interleaved (not multi-planar) pixel buffer, so that all of the pixel data
    // is in a single 2-dimensional table (plane 0). Later on, we re-interpret
    // that table as XCB image data, which isn't something we could do if we had
    // e.g. multi-planar YCbCr.
    if (!res.pixbuf.pixcfg.pixel_format().is_interleaved()) 
    {
        std::cout << "non-interleaved pixbuf " << fileName << "\n";
        return false;
    }
    wuffs_base__table_u8 tab = res.pixbuf.plane(0);
    if (tab.width != tab.stride) 
    {
        // The xcb_image_create_native call, later on, assumes that (tab.height *
        // tab.stride) bytes are readable, which isn't quite the same as what
        // wuffs_base__table__flattened_length(tab.width, tab.height, tab.stride)
        // returns unless the table is tight (its width equals its stride).
        std::cout << "could not allocate tight pixbuf " << fileName << "\n";
        return false;
    }

    g_width = res.pixbuf.pixcfg.width();
    g_height = res.pixbuf.pixcfg.height();
    g_pixbuf_mem_owner = std::move(res.pixbuf_mem_owner);
    g_pixbuf = res.pixbuf;

    if (res.error_message.empty())
    {
        std::cout << fileName << " : ok, size: " << g_width << "x" << g_height 
            << " bpp: " << res.pixbuf.pixcfg.pixel_format().bits_per_pixel() 
            << " indexed: " << res.pixbuf.pixcfg.pixel_format().is_indexed()
            << "\n";
    }
    else
    {
        std::cout << fileName << ": " << res.error_message.c_str() << "\n";
        return false;
    }

    return res.pixbuf.pixcfg.is_valid();
}

uint32_t ImgReader::width() const
{
	return g_width;
}

uint32_t ImgReader::height() const
{
	return g_height;
}
