#include <Onyx/App/Window.h>

#if defined(__APPLE__)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

// â”€â”€ Platform-specific GL hints â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Window::configureGLFW() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Always decorated on macOS; borderless via NSWindow
}

// â”€â”€ Pre-window platform setup â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Window::initNative() {
    // When launched as a bare executable (not via Finder/.app bundle) the OS
    // classifies the process as a background/CLI tool: no Dock entry, no
    // Cmd+Tab, and the titlebar shows the parent shell name instead of the
    // app name.
    //
    // TransformProcessType re-classifies the process as a regular foreground
    // GUI app at the OS level.  Must be called BEFORE glfwInit() so that the
    // Cocoa backend finds a fully-promoted NSApplication.
    // This call is a documented no-op when the process is already a foreground
    // app (i.e. launched via .app bundle), so it is always safe to call.
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);

    // Ensure NSApp exists and is initialized before GLFW touches Cocoa.
    // [NSApplication sharedApplication] is idempotent â€” safe to call even if
    // GLFW would call it anyway.  Without this, [NSApp activateIgnoringOtherApps]
    // below silently becomes a no-op because NSApp is still nil.
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Bring the app to the foreground immediately.  Without this, when launched
    // from the terminal the window opens behind the Terminal window.
    [NSApp activateIgnoringOtherApps:YES];
}

// â”€â”€ Post-window platform setup â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Merges the old NativeWindow::setup() functionality
void Window::setupNativeWindow() {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(m_window);
    cocoaWindow.titleVisibility = NSWindowTitleHidden;

    bool borderless = isBorderless();

    if (borderless) {
        cocoaWindow.titlebarAppearsTransparent = YES;
        cocoaWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;

        // Glass/vibrancy background effect (cloned from ImHex)
        {
            NSView* glfwContentView = [cocoaWindow contentView];

            NSOpenGLContext* context = [NSOpenGLContext currentContext];
            if (!context) {
                glfwMakeContextCurrent(m_window);
                context = [NSOpenGLContext currentContext];
            }

            GLint opaque = 0;
            [context setValues:&opaque forParameter:NSOpenGLCPSurfaceOpacity];
            [context update];

            NSView* containerView = [[NSView alloc] initWithFrame:[glfwContentView frame]];
            containerView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [containerView setWantsLayer:YES];

            // Try new liquid glass effect first, fall back to NSVisualEffectView
            Class glassEffectClass = NSClassFromString(@"NSGlassEffectView");
            NSView* effectView = nil;
            if (glassEffectClass) {
                effectView = [[glassEffectClass alloc] initWithFrame:[containerView bounds]];
            } else {
                NSVisualEffectView* visualEffectView = [[NSVisualEffectView alloc]
                    initWithFrame:[containerView bounds]];
                visualEffectView.material = NSVisualEffectMaterialHUDWindow;
                visualEffectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
                visualEffectView.state = NSVisualEffectStateActive;
                effectView = visualEffectView;
            }

            effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [containerView addSubview:effectView];

            [glfwContentView removeFromSuperview];
            glfwContentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [containerView addSubview:glfwContentView];

            [cocoaWindow setContentView:containerView];
        }

        [cocoaWindow setOpaque:NO];
        [cocoaWindow setHasShadow:YES];
        [cocoaWindow setBackgroundColor:[NSColor colorWithWhite:0 alpha:0.001f]];
    }
}

// â”€â”€ Per-frame platform hooks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Window::beginNativeWindowFrame() {
    // No-op for now; macOS handles titlebar natively
}

void Window::endNativeWindowFrame() {
    // No-op
}

#pragma clang diagnostic pop

#endif
