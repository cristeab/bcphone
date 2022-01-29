#include "dock_click_handler.h"
#define OBJC_OLD_DISPATCH_PROTOTYPES 1
#import <Foundation/Foundation.h>
#import <objc/objc-class.h>
#import <Foundation/NSObjCRuntime.h>
#include <QDebug>

static QObject* g_rootObject = nullptr;
int dockClickHandler(id self,SEL _cmd,...) {
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    QMetaObject::invokeMethod(g_rootObject, "raiseWindow");
    return NO;//suppress the default macOS action
}
void setupDockClickHandler(QObject *rootObject) {
    g_rootObject = rootObject;
    Class cls = objc_getClass("NSApplication");
    id appInst = objc_msgSend((id)cls, sel_registerName("sharedApplication"));
    if(appInst) {
        id delegate = objc_msgSend(appInst, sel_registerName("delegate"));
        Class delClass = (Class)objc_msgSend(delegate,  sel_registerName("class"));
        SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
        if (class_getInstanceMethod(delClass, shouldHandle)) {
            if (class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:")) {
                qDebug() << "Registered quit click handler (replaced original method)";
            }
            else {
                qWarning() << "Failed to replace method for quit click handler";
            }
        }
        else {
            if (class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:")) {
                qDebug() << "Registered quit click handler";
            }
            else {
                qWarning() << "Failed to register quit click handler";
            }
        }
    }
}
