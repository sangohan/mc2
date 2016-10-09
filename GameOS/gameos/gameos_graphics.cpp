#include "gameos.hpp"
#include "font3d.hpp"
#include <vector>
#include <map>

#ifdef LINUX_BUILD
#include <cstdarg>
#endif

struct gosTextureInfo {
    int width_;
    int height_;
    gos_TextureFormat format_;
};

class gosTexture {
    public:
        gosTexture(gos_TextureFormat fmt, const char* fname, DWORD hints, BYTE* pdata, DWORD size)
        {
            format_ = fmt;
            if(fname) {
                filename_ = new char[strlen(fname)+1];
                strcpy(filename_, fname);
            } else {
                filename_ = 0;
            }
            hints_ = hints;
            
            size_ = size;
            pdata_ = new BYTE[size];
            memcpy(pdata_, pdata, size);

            is_locked_ = false;

            // TODO: correctly set width and height
            width_ = 16;
            height_ = 16;
            
        }
        ~gosTexture() {
            delete[] pdata_;
            delete[] filename_;
        }

        BYTE* Lock(int mipl_level, float is_read_only, int* pitch) {
            gosASSERT(is_locked_ == false);
            is_locked_ = true;
            // TODO:
            gosASSERT(pitch);
            *pitch = width_;
            return pdata_;
        }

        void Unlock() {
            gosASSERT(is_locked_ == true);
            is_locked_ = false;
        }

        void getTextureInfo(gosTextureInfo* texinfo) {
            gosASSERT(texinfo);
            texinfo->width_ = width_;
            texinfo->height_ = height_;
            texinfo->format_ = format_;
        }

    private:
        int width_;
        int height_;

        BYTE* pdata_;
        DWORD size_;

        gos_TextureFormat format_;
        char* filename_;
        DWORD hints_;

        bool is_locked_;
};

struct gosTextAttribs {
    HGOSFONT3D FontHandle;
    DWORD Foreground;
    float Size;
    bool WordWrap;
    bool Proportional;
    bool Bold;
    bool Italic;
    DWORD WrapType;
    bool DisableEmbeddedCodes;
};

class gosRenderer {

    typedef std::map<gos_RenderState, unsigned int> StatePair;
    typedef std::vector<StatePair> StateList;

    public:
        DWORD addTexture(gosTexture* texture) {
            gosASSERT(texture);
            textureList_.push_back(texture);
            return textureList_.size()-1;
        }

        gosTexture* getTexture(DWORD texture_id) {
            gosASSERT(textureList_.size() > texture_id);
            gosASSERT(textureList_[texture_id] != 0);
            return textureList_[texture_id];
        }

        void deleteTexture(DWORD texture_id) {
            // FIXME: bad use object list, with stable ids
            // to not waste space
            gosASSERT(textureList_.size() > texture_id);
            delete textureList_[texture_id];
            textureList_[texture_id] = 0;
        }

        gosTextAttribs& getTextAttributes() { return curTextAttribs_; }
        void setTextPos(int x, int y) { curTextPosX_ = x; curTextPosY_ = y; }
        void setTextRegion(int Left, int Top, int Right, int Bottom) {
            curTextLeft_ = Left;
            curTextTop_ = Top;
            curTextRight_ = Right;
            curTextBottom_ = Bottom;
        }

        void init();

    private:
        void initRenderStates();

        std::vector<gosTexture*> textureList_;

        // states data
        unsigned int curStates_[gos_MaxState];

        int statesStackPointer;
        StateList statesStack_[16];
        //

        // text data
        gosTextAttribs curTextAttribs_;

        int curTextPosX_;
        int curTextPosY_;

        int curTextLeft_;
        int curTextTop_;
        int curTextRight_;
        int curTextBottom_;
        //
};

void gosRenderer::init() {
    initRenderStates();
}

void gosRenderer::initRenderStates() {

	curStates_[gos_State_Texture] = 0;
	curStates_[gos_State_Texture2] = 0;
    curStates_[gos_State_Texture3] = 0;
	curStates_[gos_State_Filter] = gos_FilterNone;
	curStates_[gos_State_ZCompare] = 1; 
    curStates_[gos_State_ZWrite] = 1;
	curStates_[gos_State_AlphaTest] = 0;
	curStates_[gos_State_Perspective] = 1;
	curStates_[gos_State_Specular] = 0;
	curStates_[gos_State_Dither] = 0;
	curStates_[gos_State_Clipping] = 0;	
	curStates_[gos_State_WireframeMode] = 0;
	curStates_[gos_State_AlphaMode] = gos_Alpha_OneZero;
	curStates_[gos_State_TextureAddress] = gos_TextureWrap;
	curStates_[gos_State_ShadeMode] = gos_ShadeGouraud;
	curStates_[gos_State_TextureMapBlend] = gos_BlendModulateAlpha;
	curStates_[gos_State_MipMapBias] = 0;
	curStates_[gos_State_Fog]= 0;
	curStates_[gos_State_MonoEnable] = 0;
	curStates_[gos_State_Culling] = gos_Cull_None;
	curStates_[gos_State_StencilEnable] = 0;
	curStates_[gos_State_StencilFunc] = gos_Cmp_Never;
	curStates_[gos_State_StencilRef] = 0;
	curStates_[gos_State_StencilMask] = 0xffffffff;
	curStates_[gos_State_StencilZFail] = gos_Stencil_Keep;
	curStates_[gos_State_StencilFail] = gos_Stencil_Keep;
	curStates_[gos_State_StencilPass] = gos_Stencil_Keep;
	curStates_[gos_State_Multitexture] = gos_Multitexture_None;
	curStates_[gos_State_Ambient] = 0xffffff;
	curStates_[gos_State_Lighting] = 0;
	curStates_[gos_State_NormalizeNormals] = 0;
	curStates_[gos_State_VertexBlend] = 0;
}

static gosRenderer* g_gos_renderer = NULL;

void gos_CreateRenderer()
{
    g_gos_renderer = new gosRenderer();
    g_gos_renderer->init();
}

////////////////////////////////////////////////////////////////////////////////
// graphics
//
void _stdcall gos_DrawLines(gos_VERTEX* Vertices, int NumVertices)
{
    gosASSERT(0 && "Not implemented");
}
void _stdcall gos_DrawPoints(gos_VERTEX* Vertices, int NumVertices)
{
    gosASSERT(0 && "Not implemented");
}
void _stdcall gos_DrawQuads(gos_VERTEX* Vertices, int NumVertices)
{
    gosASSERT(0 && "Not implemented");
}
void _stdcall gos_DrawTriangles(gos_VERTEX* Vertices, int NumVertices)
{
    gosASSERT(0 && "Not implemented");
}

void __stdcall gos_GetViewport( float* pViewportMulX, float* pViewportMulY, float* pViewportAddX, float* pViewportAddY )
{
    gosASSERT(0 && "Not implemented");
}
/*
typedef struct _FontInfo
	DWORD		MagicNumber;			// Valid font check
	_FontInfo*	pNext;					// Pointer to next font
	DWORD		ReferenceCount;			// Reference count
	char		FontFile[MAX_PATH];		// Path name of font texture
	DWORD		StartLine;				// texture line where font starts
	int			CharCount;				// number of chars in font (valid range 33 to 256)
	DWORD		TextureSize;			// Width and Height of texture
	float		rhSize;					// 1.0 / Size
	DWORD		TexturePitch;			// Pitch of texture
	int			Width;					// Width of font grid
	int			Height;					// Height of font grid
	DWORD		Across;					// Number of characters across one line
	DWORD		Aliased;				// True if 4444 texture (may be aliased - else 1555 keyed)
	DWORD		FromTextureHandle;		// True is from a texture handle
	BYTE		BlankPixels[256-32];	// Empty pixels before character
	BYTE		UsedPixels[256-32];		// Width of character
	BYTE		TopU[256-32];
	BYTE		TopV[256-32];			// Position of character
	BYTE		TopOffset[256-32];		// Offset from top (number of blank lines)
	BYTE		RealHeight[256-32];		// Height of character
	BYTE		TextureHandle[256-32];	// Which texture handle to use
	DWORD		NumberOfTextures;		// Number of texture handles used (normally 1)
	HFONT		hFontTTF;				// handle to a GDI font
	DWORD		Texture[8];				// Texture handle array
*/
HGOSFONT3D __stdcall gos_LoadFont( const char* FontFile, DWORD StartLine/* = 0*/, int CharCount/* = 256*/, DWORD TextureHandle/*=0*/)
{
    gosASSERT(FontFile);
    _FontInfo* fi = new _FontInfo();
    memset(fi, 0, sizeof(fi));
    strncpy(fi->FontFile, FontFile, sizeof(fi->FontFile));
    fi->StartLine = StartLine;
    fi->CharCount = CharCount;
    fi->NumberOfTextures = TextureHandle!=0 ? 1 : 0;
    fi->TextureHandle[0] = TextureHandle;

    // TODO: actually load some font
    fi->Width = 16;
    fi->Height = 16;

    //gosASSERT(0 && "Not implemented");
    return fi;
}

void __stdcall gos_DeleteFont( HGOSFONT3D Fonthandle )
{
    gosASSERT(Fonthandle);
    delete (_FontInfo*)Fonthandle;
}

DWORD __stdcall gos_NewEmptyTexture( gos_TextureFormat Format, const char* Name, DWORD HeightWidth, DWORD Hints/*=0*/, gos_RebuildFunction pFunc/*=0*/, void *pInstance/*=0*/)
{
    gosASSERT(0 && "Not implemented");
    return -1;
}
DWORD __stdcall gos_NewTextureFromMemory( gos_TextureFormat Format, const char* FileName, BYTE* pBitmap, DWORD Size, DWORD Hints/*=0*/, gos_RebuildFunction pFunc/*=0*/, void *pInstance/*=0*/)
{
    //gosASSERT(0 && "Not implemented");

    gosTexture* ptex = new gosTexture(Format, FileName, Hints, pBitmap, Size);
    return g_gos_renderer->addTexture(ptex);
}
void __stdcall gos_DestroyTexture( DWORD Handle )
{
    //gosASSERT(0 && "Not implemented");
    g_gos_renderer->deleteTexture(Handle);
}

void __stdcall gos_LockTexture( DWORD Handle, DWORD MipMapSize, bool ReadOnly, TEXTUREPTR* TextureInfo )
{
    // not implemented yet
    gosASSERT(MipMapSize == 0);
    int mip_level = 0; //func(MipMapSize);

    gosTextureInfo info;
    int pitch = 0;
    gosTexture* ptex = g_gos_renderer->getTexture(Handle);
    ptex->getTextureInfo(&info);
    BYTE* pdata = ptex->Lock(mip_level, ReadOnly, &pitch);

    TextureInfo->pTexture = (DWORD*)pdata;
    TextureInfo->Width = info.width_;
    TextureInfo->Height = info.height_;
    TextureInfo->Pitch = pitch;
    TextureInfo->Type = info.format_;

    //gosASSERT(0 && "Not implemented");
}

void __stdcall gos_UnLockTexture( DWORD Handle )
{
    gosTexture* ptex = g_gos_renderer->getTexture(Handle);
    ptex->Unlock();

    //gosASSERT(0 && "Not implemented");
}

void __stdcall gos_PushRenderStates()
{
    gosASSERT(0 && "not implemented");
}
void __stdcall gos_PopRenderStates()
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_RenderIndexedArray( gos_VERTEX* pVertexArray, DWORD NumberVertices, WORD* lpwIndices, DWORD NumberIndices )
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_RenderIndexedArray( gos_VERTEX_2UV* pVertexArray, DWORD NumberVertices, WORD* lpwIndices, DWORD NumberIndices )
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_SetRenderState( gos_RenderState RenderState, int Value )
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_SetScreenMode( DWORD Width, DWORD Height, DWORD bitDepth/*=16*/, DWORD Device/*=0*/, bool disableZBuffer/*=0*/, bool AntiAlias/*=0*/, bool RenderToVram/*=0*/, bool GotoFullScreen/*=0*/, int DirtyRectangle/*=0*/, bool GotoWindowMode/*=0*/, bool EnableStencil/*=0*/, DWORD Renderer/*=0*/)
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_SetupViewport( bool FillZ, float ZBuffer, bool FillBG, DWORD BGColor, float top, float left, float bottom, float right, bool ClearStencil/*=0*/, DWORD StencilValue/*=0*/)
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_TextDraw( const char *Message, ... )
{
    gosASSERT(0 && "not implemented");
}

void __stdcall gos_TextSetAttributes( HGOSFONT3D FontHandle, DWORD Foreground, float Size, bool WordWrap, bool Proportional, bool Bold, bool Italic, DWORD WrapType/*=0*/, bool DisableEmbeddedCodes/*=0*/)
{
    gosASSERT(g_gos_renderer);

    gosTextAttribs& ta = g_gos_renderer->getTextAttributes();
    ta.FontHandle = FontHandle;
    ta.Foreground = Foreground;
    ta.Size = Size;
    ta.WordWrap = WordWrap;
    ta.Proportional = Proportional;
    ta.Bold = Bold;
    ta.Italic = Italic;
    ta.WrapType = WrapType;
    ta.DisableEmbeddedCodes = DisableEmbeddedCodes;
}

void __stdcall gos_TextSetPosition( int XPosition, int YPosition )
{
    gosASSERT(g_gos_renderer);
    g_gos_renderer->setTextPos(XPosition, YPosition);
}

void __stdcall gos_TextSetRegion( int Left, int Top, int Right, int Bottom )
{
    gosASSERT(g_gos_renderer);
    g_gos_renderer->setTextRegion(Left, Top, Right, Bottom);
}

void __stdcall gos_TextStringLength( DWORD* Width, DWORD* Height, const char *fmt, ... )
{
    gosASSERT(Width && Height && fmt);

    const int   MAX_TEXT_LEN = 4096;
	char        text[MAX_TEXT_LEN] = {0};
	va_list	    ap;

    va_start(ap, fmt);
	vsnprintf(text, MAX_TEXT_LEN - 1, fmt, ap);
    va_end(ap);

	size_t len = strlen(text);
    text[len] = '\0';

    int num_newlines = 0;
    int max_width = 0;
    int cur_width = 0;
    const char* txtptr = text;
    while(*txtptr) {
        if(*txtptr++ == '\n') {
            num_newlines++;
            max_width = max_width > cur_width ? max_width : cur_width;
            cur_width = 0;
        } else {
            cur_width++;
        }
    }
    max_width = max_width > cur_width ? max_width : cur_width;

    int w = max_width;
    int h = num_newlines + 1;

    const gosTextAttribs& ta = g_gos_renderer->getTextAttributes();
    _FontInfo* fi = ta.FontHandle;
    gosASSERT(fi);
    w *= fi->Width;
    h *= fi->Height;

    *Width = w;
    *Height = h;

}

