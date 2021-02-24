#include <JavaScriptCore/JavaScript.h>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "kraken_bridge_jsc_config.h"

using JSExceptionHandler = std::function<void(int32_t contextId, const char *errmsg)>;

class NativeString;

namespace kraken::binding::jsc {

class JSContext;
class JSFunctionHolder;
class JSStringHolder;
class JSValueHolder;
class HostObject;
template <typename T> class JSHostObjectHolder;
class HostClass;
class JSEvent;
class EventInstance;
class NativeEvent;
class NativeEventTarget;
class JSEventTarget;
class EventTargetInstance;
class JSNode;
class NodeInstance;
struct NativeNode;
class JSDocument;
class DocumentCookie;
class DocumentInstance;
struct NativeDocument;
class CSSStyleDeclaration;
class JSElementAttributes;
class JSElement;
class ElementInstance;
struct NativeBoundingClientRect;
class BoundingClientRect;
struct NativeElement;
class JSEvent;
struct NativeEvent;
class JSGestureEvent;
struct NativeGestureEvent;
class GestureEventInstance;

class JSContext {
public:
  static std::vector<JSStaticFunction> globalFunctions;
  static std::vector<JSStaticValue> globalValue;

  JSContext() = delete;
  JSContext(int32_t contextId, const JSExceptionHandler &handler, void *owner);
  ~JSContext();

  KRAKEN_EXPORT bool evaluateJavaScript(const uint16_t *code, size_t codeLength, const char *sourceURL, int startLine);
  KRAKEN_EXPORT bool evaluateJavaScript(const char16_t *code, size_t length, const char *sourceURL, int startLine);

  bool isValid();

  KRAKEN_EXPORT JSObjectRef global();
  KRAKEN_EXPORT JSGlobalContextRef context();

  KRAKEN_EXPORT int32_t getContextId();

  KRAKEN_EXPORT void *getOwner();

  KRAKEN_EXPORT bool handleException(JSValueRef exc);

  KRAKEN_EXPORT void reportError(const char *errmsg);

  std::chrono::time_point<std::chrono::system_clock> timeOrigin;

  int32_t uniqueId;

private:
  int32_t contextId;
  JSExceptionHandler _handler;
  void *owner;
  std::atomic<bool> ctxInvalid_{false};
  JSGlobalContextRef ctx_;
};

class JSFunctionHolder {
public:
  JSFunctionHolder() = delete;
  explicit JSFunctionHolder(JSContext *context, JSObjectRef root, void *data, const std::string &name,
                            JSObjectCallAsFunctionCallback callback);

private:
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(JSFunctionHolder);
};

class JSStringHolder {
public:
  JSStringHolder() = delete;
  KRAKEN_EXPORT explicit JSStringHolder(JSContext *context, const std::string &string);
  ~JSStringHolder();

  KRAKEN_EXPORT JSValueRef makeString();
  KRAKEN_EXPORT JSStringRef getString();
  KRAKEN_EXPORT std::string string();

  KRAKEN_EXPORT const JSChar *ptr();
  KRAKEN_EXPORT size_t utf8Size();
  KRAKEN_EXPORT size_t size();
  KRAKEN_EXPORT bool empty();

  KRAKEN_EXPORT void setString(JSStringRef value);
  KRAKEN_EXPORT void setString(NativeString *value);

private:
  JSContext *m_context;
  JSStringRef m_string{nullptr};
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(JSStringHolder);
};

class JSValueHolder {
public:
  JSValueHolder() = delete;
  KRAKEN_EXPORT explicit JSValueHolder(JSContext *context, JSValueRef value);
  ~JSValueHolder();
  KRAKEN_EXPORT JSValueRef value();

  KRAKEN_EXPORT void setValue(JSValueRef value);

private:
  JSContext *m_context;
  JSValueRef m_value{nullptr};
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(JSValueHolder);
};

void KRAKEN_EXPORT buildUICommandArgs(JSStringRef key, NativeString &args_01);
void KRAKEN_EXPORT buildUICommandArgs(std::string &key, NativeString &args_01);
void KRAKEN_EXPORT buildUICommandArgs(std::string &key, JSStringRef value, NativeString &args_01,
                                      NativeString &args_02);
void KRAKEN_EXPORT buildUICommandArgs(std::string &key, std::string &value, NativeString &args_01,
                                      NativeString &args_02);

void KRAKEN_EXPORT throwJSError(JSContextRef ctx, const char *msg, JSValueRef *exception);

KRAKEN_EXPORT NativeString *stringToNativeString(std::string &string);
KRAKEN_EXPORT NativeString *stringRefToNativeString(JSStringRef string);

class HostObject {
public:
  static JSValueRef proxyGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                     JSValueRef *exception);
  static bool proxySetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value,
                               JSValueRef *exception);
  static void proxyGetPropertyNames(JSContextRef ctx, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames);
  static void proxyFinalize(JSObjectRef obj);

  KRAKEN_EXPORT HostObject() = delete;
  KRAKEN_EXPORT HostObject(JSContext *context, std::string name);
  std::string name;

  JSContext *context;
  int32_t contextId;
  JSObjectRef jsObject;
  JSContextRef ctx;
  // The C++ object's dtor will be called when the GC finalizes this
  // object.  (This may be as late as when the JSContext is shut down.)
  // You have no control over which thread it is called on.  This will
  // be called from inside the GC, so it is unsafe to do any VM
  // operations which require a JSContext&.  Derived classes' dtors
  // should also avoid doing anything expensive.  Calling the dtor on
  // a js object is explicitly ok.  If you want to do JS operations,
  // or any nontrivial work, you should add it to a work queue, and
  // manage it externally.
  KRAKEN_EXPORT virtual ~HostObject();

  // When JS wants a property with a given name from the HostObject,
  // it will call this method.  If it throws an exception, the call
  // will throw a JS \c Error object. By default this returns undefined.
  // \return the value for the property.
  KRAKEN_EXPORT virtual JSValueRef getProperty(std::string &name, JSValueRef *exception);

  // When JS wants to set a property with a given name on the HostObject,
  // it will call this method. If it throws an exception, the call will
  // throw a JS \c Error object. By default this throws a type error exception
  // mimicking the behavior of a frozen object in strict mode.
  KRAKEN_EXPORT virtual bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception);

  KRAKEN_EXPORT virtual void getPropertyNames(JSPropertyNameAccumulatorRef accumulator);

private:
  JSClassRef jsClass;
};

template <typename T> class JSHostObjectHolder {
public:
  JSHostObjectHolder() = delete;
  explicit JSHostObjectHolder(JSContext *context, JSObjectRef root, const char *key, T *hostObject)
    : m_object(hostObject), m_context(context) {
    JSStringHolder keyStringHolder = JSStringHolder(context, key);
    JSObjectSetProperty(context->context(), root, keyStringHolder.getString(), hostObject->jsObject,
                        kJSPropertyAttributeNone, nullptr);
  }
  T *operator*() {
    return m_object;
  }

private:
  T *m_object;
  JSContext *m_context{nullptr};
};

class HostClass {
public:
  static void proxyFinalize(JSObjectRef object);
  static bool proxyHasInstance(JSContextRef ctx, JSObjectRef constructor, JSValueRef possibleInstance,
                               JSValueRef *exception);
  static JSValueRef proxyCallAsFunction(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                        size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
  static JSObjectRef proxyCallAsConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                            const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef proxyGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                     JSValueRef *exception);
  static JSValueRef proxyInstanceGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                             JSValueRef *exception);
  static JSValueRef proxyPrototypeGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                              JSValueRef *exception);
  static bool proxyInstanceSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value,
                                       JSValueRef *exception);
  static void proxyInstanceGetPropertyNames(JSContextRef ctx, JSObjectRef object,
                                            JSPropertyNameAccumulatorRef propertyNames);
  static void proxyInstanceFinalize(JSObjectRef obj);

  KRAKEN_EXPORT HostClass() = delete;
  KRAKEN_EXPORT HostClass(JSContext *context, std::string name);
  KRAKEN_EXPORT HostClass(JSContext *context, HostClass *parentHostClass, std::string name,
                          const JSStaticFunction *staticFunction, const JSStaticValue *staticValue);

  KRAKEN_EXPORT virtual JSValueRef getProperty(std::string &name, JSValueRef *exception);
  KRAKEN_EXPORT virtual JSValueRef prototypeGetProperty(std::string &name, JSValueRef *exception);

  // Triggered when this HostClass had been finalized by GC.
  KRAKEN_EXPORT virtual ~HostClass();

  KRAKEN_EXPORT virtual JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                                        const JSValueRef *arguments, JSValueRef *exception);

  // The instance class represent every javascript instance objects created by new expression.
  class Instance {
  public:
    Instance() = delete;
    KRAKEN_EXPORT explicit Instance(HostClass *hostClass);
    KRAKEN_EXPORT virtual ~Instance();
    KRAKEN_EXPORT virtual JSValueRef getProperty(std::string &name, JSValueRef *exception);
    KRAKEN_EXPORT virtual bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception);
    KRAKEN_EXPORT virtual void getPropertyNames(JSPropertyNameAccumulatorRef accumulator);

    template <typename T> T *prototype() {
      return reinterpret_cast<T *>(_hostClass);
    }

    JSObjectRef object{nullptr};
    HostClass *_hostClass{nullptr};
    JSContext *context{nullptr};
    JSContextRef ctx{nullptr};
    int32_t contextId;
  };

  static bool hasProto(JSContextRef ctx, JSObjectRef child, JSValueRef *exception);
  static JSObjectRef getProto(JSContextRef ctx, JSObjectRef child, JSValueRef *exception);
  static void setProto(JSContextRef ctx, JSObjectRef prototype, JSObjectRef child, JSValueRef *exception);

  std::string _name{""};
  JSContext *context{nullptr};
  int32_t contextId;
  JSContextRef ctx{nullptr};
  // The javascript constructor function.
  JSObjectRef classObject{nullptr};
  // The class template of javascript instance objects.
  JSClassRef instanceClass{nullptr};
  // The prototype object of this class.
  JSObjectRef prototypeObject{nullptr};
  JSObjectRef _call{nullptr};

private:
  void initPrototype() const;

  // The class template of javascript constructor function.
  JSClassRef jsClass{nullptr};
  HostClass *_parentHostClass{nullptr};
};

class JSHostClassHolder {
public:
  JSHostClassHolder() = delete;
  explicit JSHostClassHolder(JSContext *context, JSObjectRef root, const char *key, HostClass::Instance *hostClass)
    : m_object(hostClass), m_context(context) {
    JSStringHolder keyStringHolder = JSStringHolder(context, key);
    JSObjectSetProperty(context->context(), root, keyStringHolder.getString(), hostClass->object,
                        kJSPropertyAttributeNone, nullptr);
  }
  HostClass::Instance *operator*() {
    return m_object;
  }

private:
  HostClass::Instance *m_object;
  JSContext *m_context{nullptr};
};

using EventCreator = EventInstance *(*)(JSContext *context, void *nativeEvent);

class JSEvent : public HostClass {
public:
  DEFINE_OBJECT_PROPERTY(Event, 10, type, bubbles, cancelable, timestamp, defaultPrevented, target, srcElement,
                         currentTarget, returnValue, cancelBubble)
  DEFINE_STATIC_OBJECT_PROPERTY(Event, 4, __initWithNativeEvent__, stopImmediatePropagation, stopPropagation,
                                preventDefault)

  static std::unordered_map<JSContext *, JSEvent *> instanceMap;
  static std::unordered_map<std::string, EventCreator> eventCreatorMap;
  OBJECT_INSTANCE(JSEvent)
  // Create an Event Object from an nativeEvent address which allocated by dart side.
  static JSValueRef initWithNativeEvent(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                        size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  static EventInstance *buildEventInstance(std::string &eventType, JSContext *context, void *nativeEvent,
                                           bool isCustomEvent);

  static void defineEvent(std::string eventType, EventCreator creator);

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;

protected:
  JSEvent() = delete;
  explicit JSEvent(JSContext *context, const char *name);
  explicit JSEvent(JSContext *context);
  ~JSEvent() override;

private:
  JSFunctionHolder m_initWithNativeEvent{context, classObject, this, "__initWithNativeEvent__", initWithNativeEvent};
  friend EventInstance;
};

class EventInstance : public HostClass::Instance {
public:
  EventInstance() = delete;

  explicit EventInstance(JSEvent *jsEvent, NativeEvent *nativeEvent);
  explicit EventInstance(JSEvent *jsEvent, std::string eventType, JSValueRef eventInit, JSValueRef *exception);
  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;
  ~EventInstance() override;
  NativeEvent *nativeEvent;
  bool _dispatchFlag{false};
  bool _canceledFlag{false};
  bool _initializedFlag{true};
  bool _stopPropagationFlag{false};
  bool _stopImmediatePropagationFlag{false};
  bool _inPassiveListenerFlag{false};

private:
  static JSValueRef stopPropagation(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                    size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef stopImmediatePropagation(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                             size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef preventDefault(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                   const JSValueRef arguments[], JSValueRef *exception);

  friend JSEvent;
  JSFunctionHolder m_stopImmediatePropagation{context, object, this, "stopImmediatePropagation",
                                              stopImmediatePropagation};
  JSFunctionHolder m_stopPropagation{context, object, this, "stopPropagation", stopPropagation};
  JSFunctionHolder m_preventDefault{context, object, this, "preventDefault", preventDefault};
};

struct NativeEvent {
  NativeEvent() = delete;
  explicit KRAKEN_EXPORT NativeEvent(NativeString *eventType) : type(eventType){};
  NativeString *type;
  int64_t bubbles{0};
  int64_t cancelable{0};
  int64_t timeStamp{0};
  int64_t defaultPrevented{0};
  // The pointer address of target EventTargetInstance object.
  void *target{nullptr};
  // The pointer address of current target EventTargetInstance object.
  void *currentTarget{nullptr};
};

class JSEventTarget : public HostClass {
public:
  static std::unordered_map<JSContext *, JSEventTarget *> instanceMap;
  static JSEventTarget *instance(JSContext *context);
  DEFINE_OBJECT_PROPERTY(EventTarget, 1, eventTargetId)
  DEFINE_STATIC_OBJECT_PROPERTY(EventTarget, 4, addEventListener, removeEventListener, dispatchEvent,
                                __clearListeners__)

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

  JSValueRef prototypeGetProperty(std::string &name, JSValueRef *exception) override;

protected:
  JSEventTarget() = delete;
  friend EventTargetInstance;
  KRAKEN_EXPORT explicit JSEventTarget(JSContext *context, const char *name);
  KRAKEN_EXPORT explicit JSEventTarget(JSContext *context, const JSStaticFunction *staticFunction,
                                       const JSStaticValue *staticValue);
  ~JSEventTarget();

private:
  std::vector<std::string> m_jsOnlyEvents;

  static JSValueRef addEventListener(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                     size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef removeEventListener(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                        size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef dispatchEvent(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                  const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef clearListeners(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                   const JSValueRef arguments[], JSValueRef *exception);

  JSFunctionHolder m_removeEventListener{context, prototypeObject, nullptr, "removeEventListener", removeEventListener};
  JSFunctionHolder m_dispatchEvent{context, prototypeObject, nullptr, "dispatchEvent", dispatchEvent};
  JSFunctionHolder m_clearListeners{context, prototypeObject, nullptr, "__clearListeners__", clearListeners};
  JSFunctionHolder m_addEventListener{context, prototypeObject, nullptr, "addEventListener", addEventListener};
};

class EventTargetInstance : public HostClass::Instance {
public:
  EventTargetInstance() = delete;
  KRAKEN_EXPORT explicit EventTargetInstance(JSEventTarget *eventTarget);
  KRAKEN_EXPORT explicit EventTargetInstance(JSEventTarget *eventTarget, int64_t targetId);
  KRAKEN_EXPORT JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  KRAKEN_EXPORT bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  KRAKEN_EXPORT void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;
  JSValueRef getPropertyHandler(std::string &name, JSValueRef *exception);
  void setPropertyHandler(std::string &name, JSValueRef value, JSValueRef *exception);
  bool dispatchEvent(EventInstance *event);

  ~EventTargetInstance() override;
  int32_t eventTargetId;
  NativeEventTarget *nativeEventTarget{nullptr};

private:
  friend JSEventTarget;
  // TODO: use std::u16string for better performance.
  std::unordered_map<std::string, std::deque<JSObjectRef>> _eventHandlers;
  bool internalDispatchEvent(EventInstance *eventInstance);
};

using NativeDispatchEvent = void (*)(NativeEventTarget *nativeEventTarget, NativeString *eventType, void *nativeEvent,
                                     int32_t isCustomEvent);

struct NativeEventTarget {
  NativeEventTarget() = delete;
  NativeEventTarget(EventTargetInstance *_instance)
    : instance(_instance), dispatchEvent(NativeEventTarget::dispatchEventImpl){};

  KRAKEN_EXPORT static void dispatchEventImpl(NativeEventTarget *nativeEventTarget, NativeString *eventType,
                                              void *nativeEvent, int32_t isCustomEvent);

  EventTargetInstance *instance;
  NativeDispatchEvent dispatchEvent;
};

enum NodeType {
  ELEMENT_NODE = 1,
  TEXT_NODE = 3,
  COMMENT_NODE = 8,
  DOCUMENT_NODE = 9,
  DOCUMENT_TYPE_NODE = 10,
  DOCUMENT_FRAGMENT_NODE = 11
};

#define NODE_IDENTIFY 1

class JSNode : public JSEventTarget {
public:
  static std::unordered_map<JSContext *, JSNode *> instanceMap;
  static JSNode *instance(JSContext *context);
  DEFINE_OBJECT_PROPERTY(Node, 9, isConnected, firstChild, lastChild, parentNode, childNodes, previousSibling,
                         nextSibling, nodeType, textContent)
  DEFINE_STATIC_OBJECT_PROPERTY(Node, 5, appendChild, remove, removeChild, insertBefore, replaceChild)

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

  static JSValueRef appendChild(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                const JSValueRef arguments[], JSValueRef *exception);
  /**
   * The ChildNode.remove() method removes the object
   * from the tree it belongs to.
   * reference: https://dom.spec.whatwg.org/#dom-childnode-remove
   */
  static JSValueRef remove(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                           const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef removeChild(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef insertBefore(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef replaceChild(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception);

  JSValueRef prototypeGetProperty(std::string &name, JSValueRef *exception) override;

protected:
  JSNode() = delete;
  explicit JSNode(JSContext *context);
  explicit JSNode(JSContext *context, const char *name);
  ~JSNode();

private:
};

class NodeInstance : public EventTargetInstance {
public:
  NodeInstance() = delete;
  NodeInstance(JSNode *node, NodeType nodeType);
  NodeInstance(JSNode *node, NodeType nodeType, int64_t targetId);
  ~NodeInstance() override;

  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;

  bool isConnected();
  NodeInstance *firstChild();
  NodeInstance *lastChild();
  NodeInstance *previousSibling();
  NodeInstance *nextSibling();
  void internalAppendChild(NodeInstance *node);
  void internalRemove(JSValueRef *exception);
  NodeInstance *internalRemoveChild(NodeInstance *node, JSValueRef *exception);
  void internalInsertBefore(NodeInstance *node, NodeInstance *referenceNode, JSValueRef *exception);
  virtual std::string internalGetTextContent();
  virtual void internalSetTextContent(JSStringRef content, JSValueRef *exception);
  NodeInstance *internalReplaceChild(NodeInstance *newChild, NodeInstance *oldChild,
                                             JSValueRef *exception);

  NodeType nodeType;
  NodeInstance *parentNode{nullptr};
  std::vector<NodeInstance *> childNodes;

  NativeNode *nativeNode{nullptr};

  void refer();
  void unrefer();

  int32_t _referenceCount{0};
  int32_t _identify{NODE_IDENTIFY};

  DocumentInstance *document{nullptr};
  virtual void _notifyNodeRemoved(NodeInstance *node);
  virtual void _notifyNodeInsert(NodeInstance *node);

private:
  void ensureDetached(NodeInstance *node);

  JSFunctionHolder m_removeChild{context, object, this, "removeChild", JSNode::removeChild};
  JSFunctionHolder m_appendChild{context, object, this, "appendChild", JSNode::appendChild};
  JSFunctionHolder m_remove{context, object, this, "remove", JSNode::remove};
  JSFunctionHolder m_insertBefore{context, object, this, "insertBefore", JSNode::insertBefore};
  JSFunctionHolder m_replaceChild{context, object, this, "replaceChild", JSNode::replaceChild};
};

struct NativeNode {
  NativeNode() = delete;
  KRAKEN_EXPORT NativeNode(NativeEventTarget *nativeEventTarget) : nativeEventTarget(nativeEventTarget){};
  NativeEventTarget *nativeEventTarget;
};

class JSDocument : public JSNode {
public:
  static std::unordered_map<JSContext *, JSDocument *> instanceMap;
  static JSDocument *instance(JSContext *context);

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

private:
protected:
  JSDocument() = delete;
  JSDocument(JSContext *context);
  ~JSDocument();
};

class DocumentCookie {
public:
  KRAKEN_EXPORT DocumentCookie() = default;

  KRAKEN_EXPORT std::string getCookie();
  KRAKEN_EXPORT void setCookie(std::string &str);

private:
  std::unordered_map<std::string, std::string> cookiePairs;
};

struct NativeDocument {
  NativeDocument() = delete;
  KRAKEN_EXPORT explicit NativeDocument(NativeNode *nativeNode) : nativeNode(nativeNode){};

  NativeNode *nativeNode;
};

class DocumentInstance : public NodeInstance {
public:
  DEFINE_OBJECT_PROPERTY(Document, 3, nodeName, all, cookie)
  DEFINE_STATIC_OBJECT_PROPERTY(Document, 7, body, documentElement, createElement, createTextNode, createComment,
                                getElementById, getElementsByTagName)

  static DocumentInstance *instance(JSContext *context);

  static JSValueRef createElement(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                  const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef createTextNode(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                   const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef createComment(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                  const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef getElementById(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                   const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef getElementsByTagName(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                         size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  DocumentInstance() = delete;
  KRAKEN_EXPORT explicit DocumentInstance(JSDocument *document);
  KRAKEN_EXPORT ~DocumentInstance();
  KRAKEN_EXPORT JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  KRAKEN_EXPORT bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  KRAKEN_EXPORT void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;

  void removeElementById(std::string &id, ElementInstance *element);
  void addElementById(std::string &id, ElementInstance *element);

  NativeDocument *nativeDocument;
  std::unordered_map<std::string, std::vector<ElementInstance *>> elementMapById;

  ElementInstance *body;

private:
  JSFunctionHolder m_createElement{context, object, this, "createElement", createElement};
  JSFunctionHolder m_createTextNode{context, object, this, "createTextNode", createTextNode};
  JSFunctionHolder m_createComment{context, object, this, "createComment", createComment};
  JSFunctionHolder m_getElementById{context, object, this, "getElementById", getElementById};
  JSFunctionHolder m_getElementsByTagName{context, object, this, "getElementsByTagName", getElementsByTagName};
  DocumentCookie m_cookie;
};

class JSElementAttributes : public HostObject {
public:
  JSElementAttributes() = delete;
  JSElementAttributes(JSContext *context) : HostObject(context, "NamedNodeMap") {}
  ~JSElementAttributes() override;

  enum class AttributeProperty { kLength };

  static std::vector<JSStringRef> &getAttributePropertyNames();
  static const std::unordered_map<std::string, AttributeProperty> &getAttributePropertyMap();

  KRAKEN_EXPORT JSStringRef getAttribute(std::string &name);
  KRAKEN_EXPORT void setAttribute(std::string &name, JSStringRef value);
  KRAKEN_EXPORT bool hasAttribute(std::string &name);
  KRAKEN_EXPORT void removeAttribute(std::string &name);

  KRAKEN_EXPORT JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  KRAKEN_EXPORT bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  KRAKEN_EXPORT void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;

private:
  std::map<std::string, JSStringRef> m_attributes;
  std::vector<JSStringRef> v_attributes;
};

struct NativeBoundingClientRect {
  double x;
  double y;
  double width;
  double height;
  double top;
  double right;
  double bottom;
  double left;
};

class CSSStyleDeclaration : public HostClass {
public:
  static std::unordered_map<JSContext *, CSSStyleDeclaration *> instanceMap;
  static CSSStyleDeclaration *instance(JSContext *context);

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

protected:
  CSSStyleDeclaration() = delete;
  ~CSSStyleDeclaration();
  explicit CSSStyleDeclaration(JSContext *context);
};

class StyleDeclarationInstance : public HostClass::Instance {
public:
  DEFINE_STATIC_OBJECT_PROPERTY(CSSStyleDeclaration, 3, setProperty, removeProperty, getPropertyValue)

  StyleDeclarationInstance() = delete;
  StyleDeclarationInstance(CSSStyleDeclaration *cssStyleDeclaration, EventTargetInstance *ownerEventTarget);
  ~StyleDeclarationInstance();

  static JSValueRef setProperty(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef removeProperty(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                   const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef getPropertyValue(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                     size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;
  bool internalSetProperty(std::string &name, JSValueRef value, JSValueRef *exception);
  void internalRemoveProperty(JSStringRef name, JSValueRef *exception);
  JSValueRef internalGetPropertyValue(JSStringRef name, JSValueRef *exception);

private:
  std::unordered_map<std::string, JSStringRef> properties;
  const EventTargetInstance *ownerEventTarget;

  JSFunctionHolder m_setProperty{context, object, this, "setProperty", setProperty};
  JSFunctionHolder m_getPropertyValue{context, object, this, "getPropertyValue", getPropertyValue};
  JSFunctionHolder m_removeProperty{context, object, this, "removeProperty", removeProperty};
};

using ElementCreator = ElementInstance *(*)(JSContext *context);

class JSElement : public JSNode {
public:
  DEFINE_OBJECT_PROPERTY(Element, 15, nodeName, tagName, offsetLeft, offsetTop, offsetWidth, offsetHeight, clientWidth,
                         clientHeight, clientTop, clientLeft, scrollTop, scrollLeft, scrollHeight, scrollWidth,
                         children)

  DEFINE_STATIC_OBJECT_PROPERTY(Element, 12, style, attributes, getBoundingClientRect, getAttribute, setAttribute,
                                hasAttribute, removeAttribute, toBlob, click, scroll, scrollBy, scrollTo)

  enum class ElementTagName {
    kDiv,
    kSpan,
    kAnchor,
    kAnimationPlayer,
    kAudio,
    kVideo,
    kStrong,
    kPre,
    kParagraph,
    kIframe,
    kObject,
    kImage,
    kCanvas,
    kInput,
  };

  static std::unordered_map<JSContext *, JSElement *> instanceMap;
  static std::unordered_map<std::string, ElementCreator> elementCreatorMap;
  OBJECT_INSTANCE(JSElement)

  static ElementInstance *buildElementInstance(JSContext *context, std::string &tagName);

  JSValueRef prototypeGetProperty(std::string &name, JSValueRef *exception) override;

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

  static void defineElement(std::string tagName, ElementCreator creator);

protected:
  JSElement() = delete;
  explicit JSElement(JSContext *context);
  ~JSElement();

private:
  friend ElementInstance;
};

class ElementInstance : public NodeInstance {
public:
  ElementInstance() = delete;
  explicit ElementInstance(JSElement *element, const char *tagName, bool sendUICommand);
  explicit ElementInstance(JSElement *element, JSStringRef tagName, double targetId);
  ~ElementInstance();

  JSValueRef getStringValueProperty(std::string &name);
  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;
  std::string internalGetTextContent() override;
  void internalSetTextContent(JSStringRef content, JSValueRef *exception) override;

  NativeElement *nativeElement{nullptr};

  std::string tagName();

private:
  friend JSElement;
  JSStringHolder m_tagName{context, ""};

  static JSValueRef getBoundingClientRect(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                          size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);

  static JSValueRef hasAttribute(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef setAttribute(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef getAttribute(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef removeAttribute(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                    size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef toBlob(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                           const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef click(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                          const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef scroll(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                           const JSValueRef arguments[], JSValueRef *exception);
  static JSValueRef scrollBy(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount,
                             const JSValueRef arguments[], JSValueRef *exception);

  void _notifyNodeRemoved(NodeInstance *node) override;
  void _notifyChildRemoved();
  void _notifyNodeInsert(NodeInstance *insertNode) override;
  void _notifyChildInsert();
  void _didModifyAttribute(std::string &name, std::string &oldId, std::string &newId);
  void _beforeUpdateId(std::string &oldId, std::string &newId);
  JSHostObjectHolder<JSElementAttributes> m_attributes{context, object, "attributes", new JSElementAttributes(context)};
  JSHostClassHolder m_style{
    context, object, "style",
    new StyleDeclarationInstance(CSSStyleDeclaration::instance(context), this)};

  JSFunctionHolder m_getBoundingClientRect{context, object, this, "getBoundingClientRect", getBoundingClientRect};
  JSFunctionHolder m_setAttribute{context, object, this, "setAttribute", setAttribute};
  JSFunctionHolder m_getAttribute{context, object, this, "getAttribute", getAttribute};
  JSFunctionHolder m_hasAttribute{context, object, this, "hasAttribute", hasAttribute};
  JSFunctionHolder m_removeAttribute{context, object, this, "removeAttribute", removeAttribute};
  JSFunctionHolder m_toBlob{context, object, this, "toBlob", toBlob};
  JSFunctionHolder m_click{context, object, this, "click", click};
  JSFunctionHolder m_scroll{context, object, this, "scroll", scroll};
  JSFunctionHolder m_scrollTo{context, object, this, "scrollTo", scroll};
  JSFunctionHolder m_scrollBy{context, object, this, "scrollBy", scrollBy};
};

enum class ViewModuleProperty {
  offsetTop,
  offsetLeft,
  offsetWidth,
  offsetHeight,
  clientWidth,
  clientHeight,
  clientTop,
  clientLeft,
  scrollTop,
  scrollLeft,
  scrollHeight,
  scrollWidth
};
using GetViewModuleProperty = double (*)(NativeElement *nativeElement, int64_t property);
using SetViewModuleProperty = void (*)(NativeElement *nativeElement, int64_t property, double value);
using GetBoundingClientRect = NativeBoundingClientRect *(*)(NativeElement *nativeElement);
using GetStringValueProperty = NativeString *(*)(NativeElement *nativeElement, NativeString *property);
using Click = void (*)(NativeElement *nativeElement);
using Scroll = void (*)(NativeElement *nativeElement, int32_t x, int32_t y);
using ScrollBy = void (*)(NativeElement *nativeElement, int32_t x, int32_t y);

class BoundingClientRect : public HostObject {
public:
  enum BoundingClientRectProperty { kX, kY, kWidth, kHeight, kLeft, kTop, kRight, kBottom };

  static std::array<JSStringRef, 8> &getBoundingClientRectPropertyNames();
  static const std::unordered_map<std::string, BoundingClientRectProperty> &getPropertyMap();

  BoundingClientRect() = delete;
  ~BoundingClientRect() override;
  BoundingClientRect(JSContext *context, NativeBoundingClientRect *boundingClientRect);
  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;

private:
  NativeBoundingClientRect *nativeBoundingClientRect;
};

// An struct represent Element object from dart side.
struct NativeElement {
  NativeElement() = delete;
  explicit NativeElement(NativeNode *nativeNode) : nativeNode(nativeNode){};

  const NativeNode *nativeNode;

  GetViewModuleProperty getViewModuleProperty{nullptr};
  SetViewModuleProperty setViewModuleProperty{nullptr};
  GetBoundingClientRect getBoundingClientRect{nullptr};
  GetStringValueProperty getStringValueProperty{nullptr};
  Click click{nullptr};
  Scroll scroll{nullptr};
  ScrollBy scrollBy{nullptr};
};

struct NativeGestureEvent {
  NativeGestureEvent() = delete;
  explicit NativeGestureEvent(NativeEvent *nativeEvent) : nativeEvent(nativeEvent){};

  NativeEvent *nativeEvent;

  NativeString *state;

  NativeString *direction;

  double_t deltaX;

  double_t deltaY;

  double_t velocityX;

  double_t velocityY;

  double_t scale;

  double_t rotation;
};

class JSGestureEvent : public JSEvent {
public:
  DEFINE_OBJECT_PROPERTY(GestureEvent, 8, state, direction, deltaX, deltaY, velocityX, velocityY, scale, rotation)

  DEFINE_STATIC_OBJECT_PROPERTY(GestureEvent, 1, initGestureEvent);

  static std::unordered_map<JSContext *, JSGestureEvent *> instanceMap;
  OBJECT_INSTANCE(JSGestureEvent)

  JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                  const JSValueRef *arguments, JSValueRef *exception) override;

  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;

protected:
  JSGestureEvent() = delete;
  explicit JSGestureEvent(JSContext *context);
  ~JSGestureEvent() override;

private:
  friend GestureEventInstance;

  JSFunctionHolder m_initGestureEvent{context, prototypeObject, this, "initGestureEvent", initGestureEvent};

  static JSValueRef initGestureEvent(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                     size_t argumentCount, const JSValueRef arguments[], JSValueRef *exception);
};

class GestureEventInstance : public EventInstance {
public:
  GestureEventInstance() = delete;
  explicit GestureEventInstance(JSGestureEvent *jsGestureEvent, std::string GestureEventType, JSValueRef eventInit,
                                JSValueRef *exception);
  explicit GestureEventInstance(JSGestureEvent *jsGestureEvent, NativeGestureEvent *nativeGestureEvent);
  JSValueRef getProperty(std::string &name, JSValueRef *exception) override;
  bool setProperty(std::string &name, JSValueRef value, JSValueRef *exception) override;
  void getPropertyNames(JSPropertyNameAccumulatorRef accumulator) override;
  ~GestureEventInstance() override;

private:
  friend JSGestureEvent;
  JSValueHolder m_state{context, nullptr};
  JSValueHolder m_direction{context, nullptr};
  JSValueHolder m_deltaX{context, nullptr};
  JSValueHolder m_deltaY{context, nullptr};
  JSValueHolder m_velocityX{context, nullptr};
  JSValueHolder m_velocityY{context, nullptr};
  JSValueHolder m_scale{context, nullptr};
  JSValueHolder m_rotation{context, nullptr};
  NativeGestureEvent *nativeGestureEvent;
};


} // namespace kraken::binding::jsc