#include "wx/wxprec.h"
#include "gtkglareawidget.h"
#include "wx/gtk/private.h"
#include "wx/gtk/private/eventsdisabler.h"
#include "wx/gtk/private/list.h"
#include <epoxy/gl.h>

extern "C" {
static void gtk_switchbutton_clicked_callback(GtkWidget *WXUNUSED(widget), GParamSpec *WXUNUSED(pspec), wxGLAreaWidget *cb)
{
}
}

static const char *vertex_shader_code_gles =
"attribute vec4 position;\n" \
"uniform mat4 mvp;\n" \
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *fragment_shader_code_gles =
"precision mediump float;\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0;\n" \
"  gl_FragColor = mix(vec4(1.0, 0.85, 0.35, 1.0), vec4(0.2, 0.2, 0.2, 1.0), lerpVal);\n" \
"}";

static const char *vertex_shader_code_330 =
"#version 330\n" \
"\n" \
"layout(location = 0) in vec4 position;\n" \
"uniform mat4 mvp;\n"
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *vertex_shader_code_legacy =
"#version 130\n" \
"\n" \
"attribute vec4 position;\n" \
"uniform mat4 mvp;\n" \
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *fragment_shader_code_330 =
"#version 330\n" \
"\n" \
"out vec4 outputColor;\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0f;\n" \
"  outputColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2f, 0.2f, 1.0f), lerpVal);\n" \
"}";

static const char *fragment_shader_code_legacy =
"#version 130\n" \
"\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0f;\n" \
"  gl_FragColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2, 0.2f, 1.0f), lerpVal);\n" \
"}";


wxIMPLEMENT_DYNAMIC_CLASS(wxGLAreaWidget, wxControl);

bool wxGLAreaWidget::Create(wxWindow *parent, wxWindowID id,
                            const wxPoint &pos,
                            const wxSize &size, long style,
                            const wxValidator& validator,
                            const wxString &name)
{
    if (!PreCreation(parent, pos, size) ||
        !CreateBase(parent, id, pos, size, style, validator, name ))
    {
        wxFAIL_MSG(wxT("wxGLAreaWidget creation failed"));
        return false;
    }

    m_widget = gtk_gl_area_new();
    g_object_ref_sink(m_widget);

    g_signal_connect(m_widget, "realize", G_CALLBACK(realize_cb), this);
    g_signal_connect(m_widget, "unrealize", G_CALLBACK(unrealize_cb), this);
    g_signal_connect(m_widget, "render", G_CALLBACK(render_cb), this);

    m_parent->DoAddChild(this);

    PostCreation(size);

    return true;
}

void wxGLAreaWidget::GTKDisableEvents()
{
    g_signal_handlers_block_by_func(m_widget,
                                (gpointer)gtk_switchbutton_clicked_callback, this);
}

void wxGLAreaWidget::GTKEnableEvents()
{
    g_signal_handlers_unblock_by_func(m_widget,
                                (gpointer)gtk_switchbutton_clicked_callback, this);
}

void wxGLAreaWidget::SetAxisAngleValue(const int axisIndex, const int angleVal)
{
    wxCHECK_RET(m_widget != NULL, wxT("invalid switch button"));
    rotation_angles[axisIndex] = static_cast<float>(angleVal);
    Refresh(false);
}

bool wxGLAreaWidget::GetValue() const
{
    wxCHECK_MSG(m_widget != NULL, false, wxT("invalid switch button"));

    return gtk_switch_get_active(GTK_SWITCH(m_widget)) != 0;
}

void wxGLAreaWidget::DoApplyWidgetStyle(GtkRcStyle *style)
{
    GTKApplyStyle(m_widget, style);
    GtkWidget* child = gtk_bin_get_child(GTK_BIN(m_widget));
    GTKApplyStyle(child, style);

#ifndef __WXGTK4__
    wxGCC_WARNING_SUPPRESS(deprecated-declarations)
    // for buttons with images, the path to the label is (at least in 2.12)
    // GtkButton -> GtkAlignment -> GtkHBox -> GtkLabel
    if ( GTK_IS_ALIGNMENT(child) )
    {
        GtkWidget* box = gtk_bin_get_child(GTK_BIN(child));
        if ( GTK_IS_BOX(box) )
        {
            wxGtkList list(gtk_container_get_children(GTK_CONTAINER(box)));
            for (GList* item = list; item; item = item->next)
            {
                GTKApplyStyle(GTK_WIDGET(item->data), style);
            }
        }
    }
    wxGCC_WARNING_RESTORE(0)
#endif
}

wxSize wxGLAreaWidget::DoGetBestSize() const
{
    return wxSize(640, 480);
}

void wxGLAreaWidget::init_buffers(GLuint *vao_out, GLuint *buffer_out)
{
    GLuint vao, buffer;

    const GLfloat vertex_data[] = { 0.f, 0.5f, 0.f, 1.f, 0.5f, -0.366f, 0.f, 1.f, -0.5f, -0.366f, 0.f, 1.f };

    /* we only use one VAO, so we always keep it bound */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &buffer);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (vao_out != NULL)
        *vao_out = vao;

    if (buffer_out != NULL)
        *buffer_out = buffer;
}

GLuint wxGLAreaWidget::create_shader(int type, const char *src)
{
    GLuint shader;
    int status;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        int log_len;
        std::vector<char> buffer;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        buffer.resize(log_len + 1);
        glGetShaderInfoLog(shader, log_len, NULL, buffer.data());

        g_warning("Compile failure in %s shader:\n%s",
            type == GL_VERTEX_SHADER ? "vertex" : "fragment",
            buffer);

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

void wxGLAreaWidget::init_shaders(const char *vertex_shader_code, const char *fragment_shader_code, GLuint *program_out, GLuint *mvp_out)
{
    GLuint vertex, fragment;
    GLuint program = 0;
    GLuint mvp = 0;
    int status;

    vertex = create_shader(GL_VERTEX_SHADER, vertex_shader_code);
    if (vertex == 0)
    {
        *program_out = 0;
        return;
    }

    fragment = create_shader(GL_FRAGMENT_SHADER, fragment_shader_code);
    if (fragment == 0)
    {
        glDeleteShader(vertex);
        *program_out = 0;
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        int log_len;
        std::vector<char> buffer;

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

        buffer.resize(log_len + 1);
        glGetProgramInfoLog(program, log_len, NULL, buffer.data());

        g_warning("Linking failure:\n%s", buffer.data());

        glDeleteProgram(program);
        program = 0;

        goto out;
    }

    mvp = glGetUniformLocation(program, "mvp");

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

out:
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (program_out != NULL)
        *program_out = program;

    if (mvp_out != NULL)
        *mvp_out = mvp;
}

void wxGLAreaWidget::compute_mvp(float *res, float  phi, float  theta, float  psi)
{
    float x = phi * (G_PI / 180.f);
    float y = theta * (G_PI / 180.f);
    float z = psi * (G_PI / 180.f);
    float c1 = cosf(x), s1 = sinf(x);
    float c2 = cosf(y), s2 = sinf(y);
    float c3 = cosf(z), s3 = sinf(z);
    float c3c2 = c3 * c2;
    float s3c1 = s3 * c1;
    float c3s2s1 = c3 * s2 * s1;
    float s3s1 = s3 * s1;
    float c3s2c1 = c3 * s2 * c1;
    float s3c2 = s3 * c2;
    float c3c1 = c3 * c1;
    float s3s2s1 = s3 * s2 * s1;
    float c3s1 = c3 * s1;
    float s3s2c1 = s3 * s2 * c1;
    float c2s1 = c2 * s1;
    float c2c1 = c2 * c1;

    res[0] = 1.f; res[4] = 0.f;  res[8] = 0.f; res[12] = 0.f;
    res[1] = 0.f; res[5] = 1.f;  res[9] = 0.f; res[13] = 0.f;
    res[2] = 0.f; res[6] = 0.f; res[10] = 1.f; res[14] = 0.f;
    res[3] = 0.f; res[7] = 0.f; res[11] = 0.f; res[15] = 1.f;

    res[0] = c3c2;  res[4] = s3c1 + c3s2s1;  res[8] = s3s1 - c3s2c1; res[12] = 0.f;
    res[1] = -s3c2; res[5] = c3c1 - s3s2s1;  res[9] = c3s1 + s3s2c1; res[13] = 0.f;
    res[2] = s2;    res[6] = -c2s1;         res[10] = c2c1;          res[14] = 0.f;
    res[3] = 0.f;   res[7] = 0.f;           res[11] = 0.f;           res[15] = 1.f;
}

void wxGLAreaWidget::draw_triangle(wxGLAreaWidget *glArea)
{
    float mvp[16];

    g_assert(glArea->position_buffer != 0);
    g_assert(glArea->program != 0);

    compute_mvp(mvp,
        glArea->rotation_angles[X_AXIS],
        glArea->rotation_angles[Y_AXIS],
        glArea->rotation_angles[Z_AXIS]);

    glUseProgram(glArea->program);
    glUniformMatrix4fv(glArea->mvp_location, 1, GL_FALSE, &mvp[0]);

    glBindBuffer(GL_ARRAY_BUFFER, glArea->position_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void wxGLAreaWidget::realize_cb(GtkWidget *widget, gpointer user_data)
{
    const char *fragment, *vertex;
    GdkGLContext *context;

    gtk_gl_area_make_current(GTK_GL_AREA(widget));

    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL)
        return;

    context = gtk_gl_area_get_context(GTK_GL_AREA(widget));
    if (gdk_gl_context_get_use_es(context))
    {
        vertex = vertex_shader_code_gles;
        fragment = fragment_shader_code_gles;
    }
    else
    {
        if (!gdk_gl_context_is_legacy(context))
        {
            vertex = vertex_shader_code_330;
            fragment = fragment_shader_code_330;
        }
        else
        {
            vertex = vertex_shader_code_legacy;
            fragment = fragment_shader_code_legacy;
        }
    }

    wxGLAreaWidget *glArea = reinterpret_cast<wxGLAreaWidget *>(user_data);
    init_buffers(&glArea->position_buffer, NULL);
    init_shaders(vertex, fragment, &glArea->program, &glArea->mvp_location);
}

void wxGLAreaWidget::unrealize_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_gl_area_make_current(GTK_GL_AREA(widget));

    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL)
        return;

    wxGLAreaWidget *glArea = reinterpret_cast<wxGLAreaWidget *>(user_data);
    glDeleteBuffers(1, &glArea->position_buffer);
    glDeleteProgram(glArea->program);
}

gboolean wxGLAreaWidget::render_cb(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_triangle(reinterpret_cast<wxGLAreaWidget *>(user_data));

    glFlush();

    return TRUE;
}
