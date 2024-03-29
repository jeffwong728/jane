#ifndef SPAM_UI_GRAPHICS_GL_FORWARD_H
#define SPAM_UI_GRAPHICS_GL_FORWARD_H
#include <memory>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <gtk/gtk.h>
#include <vtkProperty.h>

typedef unsigned int GLuint;
typedef unsigned int GLenum;
class GLShader;
class GLProgram;
class GLCamera;
class GLTexture;
class GLDispNode;
class GLDispTree;
class GLModelTreeView;

const int kCell_Color_Index_Normal = 0;
const int kCell_Color_Index_Selected = 1;
const int kCell_Color_Index_Highlight = 2;
const int kCell_Color_Index_Selected_And_Highlight = 3;

enum PrimGeomShape
{
    kPGS_BOX = 0,
    kPGS_SPHERE = 1,
    kPGS_CYLINDER = 2,
    kPGS_GUARD
};

enum GraphicsEntityType
{
    kENTITY_TYPE_INVALID = 0,
    kENTITY_TYPE_MODEL = 1,
    kENTITY_TYPE_ASSEMBLY = 2,
    kENTITY_TYPE_PART = 3,
    kENTITY_TYPE_ACORN_BODY = 4,
    kENTITY_TYPE_WIRE_BODY = 5,
    kENTITY_TYPE_SHEET_BODY = 6,
    kENTITY_TYPE_SOLID_BODY = 7,
    kENTITY_TYPE_0D_MESH = 8,
    kENTITY_TYPE_1D_MESH = 9,
    kENTITY_TYPE_2D_MESH = 10,
    kENTITY_TYPE_3D_MESH = 11,
    kENTITY_TYPE_GUARD
};

enum GraphicsRepresentation
{
    kGREP_VTK_POINTS = VTK_POINTS,
    kGREP_VTK_WIREFRAME = VTK_WIREFRAME,
    kGREP_VTK_SURFACE = VTK_SURFACE,
    kGREP_SURFACE_WITH_EDGE,

    kGREP_GUARD
};

struct GLGUID
{
    const guint64 part1;
    const guint64 part2;

    GLGUID() : part1(0), part2(0) {}
    GLGUID(const guint64 pat1, const guint64 pat2) : part1(pat1), part2(pat2) {}
    GLGUID(const GLGUID &o) : part1(o.part1), part2(o.part2) {}
    GLGUID &operator=(const GLGUID &o) { const_cast<guint64 &>(part1) = o.part1; const_cast<guint64 &>(part2) = o.part2; return *this; }

    bool operator<(const GLGUID &o)
    {
        if (part1 == o.part1)
        {
            return part2 < o.part2;
        }
        else
        {
            return part1 < o.part1;
        }
    }

    bool operator==(const GLGUID &o)
    {
        return part1 == o.part1 && part2 == o.part2;
    }

    static const GLGUID MakeNew()
    {
        const boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return { *reinterpret_cast<const guint64*>(uuid.begin()), *reinterpret_cast<const guint64*>(uuid.begin()+8) };
    }
};

using SPGLShader            = std::shared_ptr<GLShader>;
using SPGLProgram           = std::shared_ptr<GLProgram>;
using SPGLCamera            = std::shared_ptr<GLCamera>;
using SPGLTexture           = std::shared_ptr<GLTexture>;
using SPDispNode            = std::shared_ptr<GLDispNode>;
using SPDispTree            = std::shared_ptr<GLDispTree>;
using SPGLModelTreeView     = std::shared_ptr<GLModelTreeView>;
using WPGLShader            = std::weak_ptr<GLShader>;
using WPGLProgram           = std::weak_ptr<GLProgram>;
using WPGLCamera            = std::weak_ptr<GLCamera>;
using WPGLTexture           = std::weak_ptr<GLTexture>;
using WPDispNode            = std::weak_ptr<GLDispNode>;
using WPDispTree            = std::weak_ptr<GLDispTree>;
using WPGLModelTreeView     = std::weak_ptr<GLModelTreeView>;
using SPDispNodes           = std::vector<SPDispNode>;
using GLGUIDS               = std::vector<GLGUID>;

inline bool operator<(const GLGUID &v1, const GLGUID &v2)
{
    if (v1.part1 == v2.part1)
    {
        return v1.part2 < v2.part2;
    }
    else
    {
        return v1.part1 < v2.part1;
    }
}

#endif // SPAM_UI_GRAPHICS_GL_FORWARD_H
