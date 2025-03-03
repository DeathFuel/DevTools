#include "platform.hpp"
#include "../ImGui.hpp"
#include <Geode/loader/Log.hpp>
#include <Geode/cocos/platform/CCGL.h>

using namespace geode::prelude;

static bool g_shouldPassEventsToGDButTransformed = false;
static bool g_updateBuffer = false;
static ImRect g_GDWindowRect;

ImRect& getGDWindowRect() {
    return g_GDWindowRect;
}

bool& ::shouldPassEventsToGDButTransformed() {
    return g_shouldPassEventsToGDButTransformed;
}

bool& ::shouldUpdateGDRenderBuffer() {
    return g_updateBuffer;
}

GLRenderCtx::~GLRenderCtx() {
    this->cleanup();
}

void GLRenderCtx::cleanup() {
    if (m_depthStencil) {
        glDeleteRenderbuffers(1, &m_depthStencil);
        m_depthStencil = 0;
    }
    if (m_texture) {
        log::info("deleting texture {}", m_texture);
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    if (m_buffer) {
        glDeleteFramebuffers(1, &m_buffer);
        m_buffer = 0;
    }
}

GLRenderCtx::GLRenderCtx(ImVec2 const& size) : m_size(size) {}

ImTextureID GLRenderCtx::texture() const {
    // TODO: this wont work on 64 bit
    return reinterpret_cast<ImTextureID>(m_texture);
}

ImVec2 GLRenderCtx::size() const {
    return m_size;
}

bool GLRenderCtx::begin() {
    // save currently bound fbo
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_prevDrawBuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_prevReadBuffer);

    if (!m_buffer) {
        glGenFramebuffers(1, &m_buffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
    }

    if (!m_texture) {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        static int texture_count = 0;
        texture_count++;
        log::info("new texture is {}", m_texture);
        if (texture_count > 100) {
            exit(1);
        }

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB,
            static_cast<GLsizei>(m_size.x),
            static_cast<GLsizei>(m_size.y),
            0,GL_RGB, GL_UNSIGNED_BYTE, 0
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    if (!m_depthStencil) {
        glGenRenderbuffers(1, &m_depthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencil);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
            static_cast<GLsizei>(m_size.x),
            static_cast<GLsizei>(m_size.y)
        );
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencil);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        log::error("Unable to Render to Framebuffer");
        this->cleanup();
        return false;
    }

    // bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);

    return true;
}

void GLRenderCtx::end() {
    // bind the framebuffer that was bound before us
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_prevDrawBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_prevReadBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    //glFlush();
}

