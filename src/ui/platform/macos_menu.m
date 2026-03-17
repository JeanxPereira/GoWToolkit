// macOS native menu bar integration (adapted from ImHex macos_menu.m)
// Builds the NSMenu system menu bar from ImGui-style begin/end calls.

#if defined(__APPLE__)

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

// ── Tag-based click tracking ────────────────────────────────────────────────

static const int kMenuBegin = 1;
static NSInteger s_currTag = kMenuBegin;
static NSInteger s_selectedTag = -1;

@interface GOWMenuItemHandler : NSObject
- (void)onClick:(id)sender;
@end

@implementation GOWMenuItemHandler
- (void)onClick:(id)sender {
    NSMenuItem* item = sender;
    s_selectedTag = item.tag;
}
@end

// ── Menu stack ──────────────────────────────────────────────────────────────

static NSMenu*  s_menuStack[64];
static int      s_menuStackSize = 0;

static GOWMenuItemHandler* s_handler = nil;

static bool s_constructing = false;
static bool s_resetNeeded  = true;

// ── Public C API ────────────────────────────────────────────────────────────

void macosmenu_init(void) {
    s_menuStackSize = 0;
    s_menuStack[0] = NSApp.mainMenu;
    s_menuStackSize = 1;
    s_handler = [[GOWMenuItemHandler alloc] init];
}

void macosmenu_clear(void) {
    if (!s_menuStack[0]) return;
    // Remove all items except the Application menu (index 0)
    while (s_menuStack[0].itemArray.count > 2) {
        [s_menuStack[0] removeItemAtIndex:1];
    }
    s_currTag = kMenuBegin;
}

bool macosmenu_beginMainMenuBar(void) {
    if (s_resetNeeded) {
        macosmenu_clear();
        s_resetNeeded = false;
    }
    return true;
}

void macosmenu_endMainMenuBar(void) {
    s_constructing = false;
}

bool macosmenu_beginMenu(const char* label, bool enabled) {
    NSString* title = [NSString stringWithUTF8String:label];

    NSInteger menuIndex = [s_menuStack[s_menuStackSize - 1] indexOfItemWithTitle:title];
    if (menuIndex == -1) {
        // First time seeing this menu — construct it
        s_constructing = true;

        NSMenu* newMenu = [[NSMenu alloc] init];
        newMenu.autoenablesItems = NO;
        newMenu.title = title;

        NSMenuItem* menuItem = [[NSMenuItem alloc] init];
        menuItem.title = title;
        [menuItem setSubmenu:newMenu];

        menuIndex = [s_menuStack[s_menuStackSize - 1] numberOfItems];
        if (s_menuStackSize == 1)
            menuIndex -= 1; // Insert before the Window menu placeholder

        [s_menuStack[s_menuStackSize - 1] insertItem:menuItem atIndex:menuIndex];
    }

    NSMenuItem* menuItem = [s_menuStack[s_menuStackSize - 1] itemAtIndex:menuIndex];
    if (menuItem != nil) {
        menuItem.enabled = enabled;
        s_menuStack[s_menuStackSize] = menuItem.submenu;
        s_menuStackSize += 1;
    }

    return true;
}

void macosmenu_endMenu(void) {
    s_menuStack[s_menuStackSize - 1] = nil;
    s_menuStackSize -= 1;
}

static NSUInteger modifiersFromShortcut(const char* shortcut) {
    if (!shortcut) return 0;

    NSString* str = [NSString stringWithUTF8String:shortcut];
    NSUInteger flags = 0;

    // Parse modifier strings like "Ctrl+O", "Ctrl+Shift+E", "Alt+F4"
    if ([str containsString:@"Ctrl"] || [str containsString:@"Cmd"])
        flags |= NSEventModifierFlagCommand; // Ctrl maps to Cmd on macOS
    if ([str containsString:@"Shift"])
        flags |= NSEventModifierFlagShift;
    if ([str containsString:@"Alt"] || [str containsString:@"Opt"])
        flags |= NSEventModifierFlagOption;

    return flags;
}

static NSString* keyFromShortcut(const char* shortcut) {
    if (!shortcut) return @"";

    NSString* str = [NSString stringWithUTF8String:shortcut];

    // Find the last component after the last '+'
    NSArray* parts = [str componentsSeparatedByString:@"+"];
    NSString* key = [[parts lastObject] lowercaseString];

    // Handle special keys
    if ([key isEqualToString:@"f4"]) return [NSString stringWithFormat:@"%C", (unichar)NSF4FunctionKey];

    // Single character
    if (key.length == 1) return key;

    // Comma special case
    if ([key isEqualToString:@","]) return @",";

    return @"";
}

bool macosmenu_menuItem(const char* label, const char* shortcut,
                        bool selected, bool enabled) {
    NSString* title = [NSString stringWithUTF8String:label];

    if (s_constructing) {
        NSMenuItem* menuItem = [[NSMenuItem alloc] init];
        menuItem.title = title;
        menuItem.action = @selector(onClick:);
        menuItem.target = s_handler;
        [menuItem setTag:s_currTag];
        s_currTag += 1;

        // Setup keyboard shortcut
        if (shortcut) {
            [menuItem setKeyEquivalentModifierMask:modifiersFromShortcut(shortcut)];
            menuItem.keyEquivalent = keyFromShortcut(shortcut);
        }

        [s_menuStack[s_menuStackSize - 1] addItem:menuItem];
    }

    NSInteger menuIndex = [s_menuStack[s_menuStackSize - 1] indexOfItemWithTitle:title];
    if (menuIndex >= 0 && menuIndex < [s_menuStack[s_menuStackSize - 1] numberOfItems]) {
        NSMenuItem* menuItem = [s_menuStack[s_menuStackSize - 1] itemAtIndex:menuIndex];
        if (menuItem != nil) {
            if (!s_constructing && ![title isEqualToString:menuItem.title])
                s_resetNeeded = true;

            menuItem.enabled = enabled;
            menuItem.state = selected ? NSControlStateValueOn : NSControlStateValueOff;
        }

        if (enabled && menuItem != nil && [menuItem tag] == s_selectedTag) {
            s_selectedTag = -1;
            return true;
        }
    } else {
        s_resetNeeded = true;
    }

    return false;
}

bool macosmenu_menuItemToggle(const char* label, const char* shortcut,
                              bool* p_selected, bool enabled) {
    if (macosmenu_menuItem(label, shortcut, p_selected ? *p_selected : false, enabled)) {
        if (p_selected)
            *p_selected = !(*p_selected);
        return true;
    }
    return false;
}

void macosmenu_separator(void) {
    if (s_constructing) {
        [s_menuStack[s_menuStackSize - 1] addItem:[NSMenuItem separatorItem]];
    }
}

#endif // __APPLE__
