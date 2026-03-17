// macOS native file open dialog using NSOpenPanel

#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#include <string.h>
#include <stdlib.h>

// Returns a malloc'd string with the selected file path, or NULL if cancelled.
// Caller must free() the returned string.
const char* macos_openFileDialog(const char* const* extensions, int extCount) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setTitle:@"Open Game File"];

        if (extensions && extCount > 0) {
            NSMutableArray<NSString*>* allowed = [NSMutableArray array];
            for (int i = 0; i < extCount; i++) {
                [allowed addObject:[NSString stringWithUTF8String:extensions[i]]];
            }
            // Use allowedFileTypes (deprecated but widely compatible)
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [panel setAllowedFileTypes:allowed];
            #pragma clang diagnostic pop
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            if (url) {
                const char* path = [[url path] UTF8String];
                return strdup(path);
            }
        }
    }
    return NULL;
}

#endif // __APPLE__
