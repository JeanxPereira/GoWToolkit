#include "Ui/NativeWindow.h"

#if defined(GOWTOOL_OS_MACOS)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

static void(*s_fullFrameCallback)() = nullptr;

namespace NativeWindow {

void setFullFrameCallback(void(*cb)()) {
    s_fullFrameCallback = cb;
}

void setup(GLFWwindow* window, bool borderless) {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(window);

    cocoaWindow.titleVisibility = NSWindowTitleHidden;

    if (borderless) {
        cocoaWindow.titlebarAppearsTransparent = YES;
        cocoaWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;

        // Setup glass/vibrancy background effect (cloned from ImHex)
        {
            NSView* glfwContentView = [cocoaWindow contentView];

            NSOpenGLContext* context = [NSOpenGLContext currentContext];
            if (!context) {
                glfwMakeContextCurrent(window);
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

void beginFrame(GLFWwindow*, float) {
    // No-op for now; macOS handles titlebar natively
}

void endFrame(GLFWwindow*) {
    // No-op
}

// â”€â”€ macOS-specific helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void macosSetWindowMovable(GLFWwindow* window, bool movable) {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(window);
    [cocoaWindow setMovable:movable];
}

void macosHandleTitlebarDoubleClick(GLFWwindow* window) {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(window);

    // Respect user's system preference:
    // "System Settings -> Desktop & Dock -> Double-click a window's title bar to"
    NSString* action = [[NSUserDefaults standardUserDefaults]
        stringForKey:@"AppleActionOnDoubleClick"];

    if (action == nil || [action isEqualToString:@"None"]) {
        // Nothing
    } else if ([action isEqualToString:@"Minimize"]) {
        if ([cocoaWindow isMiniaturizable]) {
            [cocoaWindow miniaturize:nil];
        }
    } else if ([action isEqualToString:@"Maximize"]) {
        // Schedule zoom for next runloop iteration to avoid interfering with renderer
        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
            if ([cocoaWindow isZoomable]) {
                [cocoaWindow zoom:nil];
            }
        });
    }
}

bool macosIsWindowBeingResized(GLFWwindow* window) {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(window);
    return cocoaWindow.inLiveResize;
}

bool macosIsFullScreen(GLFWwindow* window) {
    NSWindow* cocoaWindow = (NSWindow*)glfwGetCocoaWindow(window);
    return (cocoaWindow.styleMask & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen;
}

} // namespace NativeWindow

#pragma clang diagnostic pop

#endif
