"""Boost.CallableTraits: libs/callable_traits/example/*.cpp (examples.cpp).
Compile-time checks: each example's body becomes a case, its feature guards
resolved for a C++23 compiler (unifdef below)."""
import re

import common as c

D = c.lib_dir("callable_traits") / "example"
DEF={'BOOST_CLBL_TRTS_ENABLE_NOEXCEPT_TYPES':True}
def defined(n): return DEF.get(n,False)
def cond(expr):
    e=expr.strip()
    e=re.sub(r'defined\s*\(\s*(\w+)\s*\)', lambda m: '1' if defined(m.group(1)) else '0', e)
    e=re.sub(r'\b(BOOST_\w+)\b', lambda m: '1' if defined(m.group(1)) else '0', e)
    e=e.replace('!','not ').replace('&&',' and ').replace('||',' or ')
    return bool(eval(e))
def unifdef(lines):
    out=[]; stack=[]  # (active_parent, taken, active)
    active=True
    for l in lines:
        s=l.strip()
        m=re.match(r'#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)',s)
        if m:
            k,rest=m.groups()
            if k in('ifdef','ifndef','if'):
                v = defined(rest.strip()) if k=='ifdef' else (not defined(rest.strip()) if k=='ifndef' else cond(rest))
                stack.append((active, v)); active = active and v
            elif k=='elif':
                par,taken=stack[-1]; v=(not taken) and cond(rest)
                stack[-1]=(par, taken or v); active=par and v
            elif k=='else':
                par,taken=stack[-1]; active=par and not taken; stack[-1]=(par,True)
            else:
                active=stack.pop()[0]
            continue
        if active: out.append(l)
    return out


HEADER = '''// mcpplibs.mwrap.boost.callable_traits — the %d official examples
// (libs/callable_traits/example/*.cpp) as test cases, each in its own
// namespace. They are compile-time checks: the test passes by compiling.
// The examples' feature guards are resolved for a C++23 compiler:
// BOOST_CLBL_TRTS_DISABLE_* and _MSVC are off, noexcept function types are on,
// and transaction_safe (GCC's -fgnu-tm) is off: the three *_transaction_safe
// examples compile to an empty main upstream then, and are excluded in
// coverage.toml.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.callable_traits;

'''


def run(group):
    res=[]
    names=[]
    for f in sorted(p for p in D.glob('*.cpp') if 'transaction_safe' not in p.stem):
        t=f.read_text()
        t='\n'.join(unifdef(t.splitlines()))
        m=re.search(r'^//\[[^\n]*\n(.*?)^//\]',t,re.S|re.M)
        body=m.group(1) if m else re.sub(r'(?s)^/\*.*?\*/','',t)
        lines=body.splitlines()
        lines=[l for l in lines if not re.match(r'\s*#\s*include',l)]
        txt='\n'.join(lines)
        name=f.stem
        if 'int main' not in txt:
            txt += '\nTEST_CASE("example: %s") {}' % name
        txt=re.sub(r'int\s+main\s*\(\s*\)\s*\{\s*\}', 'TEST_CASE("example: %s") {}'%name, txt)
        txt=re.sub(r'int\s+main\s*\(\s*\)\s*\{', 'TEST_CASE("example: %s") {'%name, txt)
        txt=re.sub(r'^\s*return 0;\s*$','',txt,flags=re.M)
        txt=re.sub(r'\n{3,}','\n\n',txt).strip('\n')
        res.append('// libs/callable_traits/example/%s.cpp\nnamespace ex_%s {\n%s\n}  // namespace ex_%s\n'%(name,name,txt,name))
        names.append(name)
    (c.TESTS / 'callable_traits/examples.cpp').write_text(HEADER % len(list(D.glob('*.cpp'))) +'\n'.join(res))
    print(len(names), 'examples')
    return 0
