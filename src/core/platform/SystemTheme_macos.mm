#include "SystemTheme.h"
#import <AppKit/AppKit.h>

namespace GOW::Platform {

SystemAppearance DetectSystemAppearance() {
    // NSAppearanceNameDarkAqua requires macOS 10.14+. Older systems are Light-only.
    if (@available(macOS 10.14, *)) {
        NSAppearance *appearance = [NSApp effectiveAppearance];
        if (appearance) {
            NSString *match = [appearance bestMatchFromAppearancesWithNames:@[
                NSAppearanceNameAqua,
                NSAppearanceNameDarkAqua
            ]];
            if ([match isEqualToString:NSAppearanceNameDarkAqua]) {
                return SystemAppearance::Dark;
            }
            return SystemAppearance::Light;
        }
    }
    return SystemAppearance::Dark; // safe fallback
}

} // namespace GOW::Platform
