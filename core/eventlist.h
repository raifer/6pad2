#ifndef _____EVENTLIST_H12
#define _____EVENTLIST_H12
#include "global.h"
#include "python34.h"
#include<functional>
#include<unordered_map>

struct EventCallback {
const PyCallback pycb;
const void* stdcb;
EventCallback (const PyCallback& cb): pycb(cb), stdcb(0) {}
bool operator== (const EventCallback& cb) { return cb.pycb==pycb && cb.stdcb==stdcb; }
template<class R, class... A> EventCallback (const std::function<R(A...)>& cb): pycb(), stdcb(&cb) {}
template<class... A> EventCallback (const std::function<void(A...)>& cb): pycb(), stdcb(&cb) {}
template<class R,class... A> R operator() (A... args) {
if (pycb) return pycb.operator()<R>(args...);
else if (stdcb) {
typedef std::function<R(A...)> Func;
Func f = *(Func*)stdcb;
return f(args...);
}
else return R();
}
template<class... A> void operator() (A... args) {
if (pycb) pycb(args...);
else if (stdcb) {
typedef std::function<void(A...)> Func;
Func f = *(Func*)stdcb;
f(args...);
}}
};

struct export eventlist {
std::unordered_multimap<std::string, EventCallback> m;
eventlist();
void add (const std::string& type, const EventCallback& cb);
bool remove (const std::string& type, const EventCallback& cb);
int count (const std::string& type);

template<class R, R initial, class... A> R dispatch (const string& type, A... args) {
R r = initial;
auto p = m.equal_range(type);
for (auto it = p.first; it!=p.second; ++it) {
r = (it->second).operator()<R>(args...);
}
return r;
}

template<class... A> var dispatch (const string& type, const var& def, A... args) {
var r = def;
auto p = m.equal_range(type);
for (auto it = p.first; it!=p.second; ++it) {
r = (it->second).operator()<var>(args...);
}
return r;
}

template<class... A> void dispatch (const string& type, A... args) {
auto p = m.equal_range(type);
for (auto it = p.first; it!=p.second; ++it) {
(it->second)(args...);
}}

};

#endif