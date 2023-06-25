#include <cMetaInterval.h>
#include <waitInterval.h>
#include <cLerpAnimEffectInterval.h>
#include <initializer_list>
#include <functional>

// I am tired of typing this all the time:
// Loads at location offset by sample_path, rooted at standard model stash
// Assumes the PandaFramework is called framework, and main win is window
#define def_load_model(x) window->load_model(framework.get_models(), sample_path + x)
#define def_load_texture(x) TexturePool::load_texture(sample_path + x)

// Support for Intervals, in the style of the Python examples
// Just prepend new and use -> if needed.  For Sequence/Parallel, enclose
// the list in additional curly braces:
// (new Sequence({ new Wait(3), new Func(std::cerr<<"hi\n")})->start();
// Since this uses direct, you will need to add p3direct to your libraries.
// You will also have to add a task to call step(), or just call:
void init_interval(void);

// The Sequence and Parallel classes take initializer lists as
// constructor arguments.  I guess panda3d is too old to support
// that itself (C++11).
typedef std::initializer_list<CInterval *> CIntervalInitList;

class Sequence : public CMetaInterval {
  private:
    void init(const CIntervalInitList ints) {
	for(auto i : ints)
	    add_c_interval(i);
    }
  public:
    Sequence(const std::string &name, const CIntervalInitList ints) :
	CMetaInterval(name) { init(ints); }
    Sequence(const CIntervalInitList ints) :
	CMetaInterval(std::to_string((unsigned long)this)) { init(ints); }
};

// Only difference between this and Sequence is relatve-to.
class Parallel : public CMetaInterval {
  private:
    void init(const CIntervalInitList ints) {
	for(auto t : ints)
	    add_c_interval(t, 0, RS_previous_begin);
    }
  public:
    Parallel(const std::string &name, const CIntervalInitList ints) :
	CMetaInterval(name) { init(ints); }
    Parallel(const CIntervalInitList ints) :
	CMetaInterval(std::to_string((unsigned long)this)) { init(ints); }
};
// Wait is just a shortcut for WaitInterval.
typedef WaitInterval Wait;
// FuncI takes a function pointer and executes it (single-shot, instant).
class FuncI : public CInterval {
  public:
    typedef std::function<void()> fp;
    FuncI(const std::string &name, fp fn, bool open = true) : CInterval(name, 0, open), _f(fn) { }
    FuncI(fp fn, bool open = true) : CInterval(std::to_string((unsigned long)this), 0, open), _f(fn) { }
    virtual void priv_instant() { _f(); }
    // NOTE: adding members requires this:
    ALLOC_DELETED_CHAIN(FuncI);
  private:
    fp _f;
};
// Func is short-hand for the most common usage: just provide call as arg.
#define Func(f) FuncI([=]{ f; })
// Linear interpolator with setter function.  Works on any type that supports
// self-addition and scaling (multiplication by a double).
// This whole thing is way too heavy for linear interpolation, but I
// will use it anyway, because it's official.
template<class C, class T>class LerpFunc : public CLerpInterval {
  public:
    LerpFunc(std::string name, C *what, void(C::*setter)(T) , T start, T end,
	     double duration) :
	CLerpInterval(name, duration, BT_no_blend),
	_what(what), _setter(setter), _start(start), _diff(end - start) {}
    LerpFunc(C *what, void(C::*setter)(T), T start, T end, double duration) :
	CLerpInterval(std::to_string((unsigned long)this), duration, BT_no_blend),
	_what(what), _setter(setter), _start(start), _diff(end - start) {}
    virtual void priv_step(double t) {
	double curt = compute_delta(t);
	if(curt >= 1)
	    (_what->*_setter)(_start + _diff);
	else
	    (_what->*_setter)(_start + _diff * curt);
    }
    // NOTE: adding members requires this:
    ALLOC_DELETED_CHAIN(LerpFunc);
  protected:
    C *_what;
    void(C::*_setter)(T);
    T _start, _diff;
};
// Linear interpolator with non-class (global) setter function.
template<class C, class T>class LerpFuncG : public CLerpInterval {
  public:
    LerpFuncG(std::string name, C setter, T start, T end,
	      double duration) :
	CLerpInterval(name, duration, BT_no_blend),
	_setter(setter), _start(start), _diff(end - start) {}
    LerpFuncG(C setter, T start, T end, double duration) :
	CLerpInterval(std::to_string((unsigned long)this), duration, BT_no_blend),
	_setter(setter), _start(start), _diff(end - start) {}
    virtual void priv_step(double t) {
	double curt = compute_delta(t);
	if(curt >= 1)
	    _setter(_start + _diff);
	else
	    _setter(_start + _diff * curt);
    }
    // NOTE: adding members requires this:
    ALLOC_DELETED_CHAIN(LerpFuncG);
  protected:
    C _setter;
    T _start, _diff;
};

// Character animations.  Instead of supplying the animation name, like in
// Python, provide the actual AnimBundle.  If you con't have it, you can still
// search by name with find().
class CharAnimate : public CLerpAnimEffectInterval {
    void init(AnimControl *ctrl, double rate, double start, double end)
    {
	auto bundle = ctrl->get_anim();
	if(end > bundle->get_num_frames() || end < 0)
	    end = bundle->get_num_frames();
	if(start < 0)
	    start = 0;
	_end_t = _duration = (end - start) / rate / bundle->get_base_frame_rate();
	set_play_rate(rate);
	add_control(ctrl, ctrl->get_name(), start, end);
    }
  public:
    CharAnimate(std::string name, AnimControl *ctrl, double rate = 1.0,
		double start = 0, double end = -1) :
	CLerpAnimEffectInterval(name, 0, BT_no_blend) {
	    init(ctrl, rate, start, end);
	}
    CharAnimate(AnimControl *ctrl, double rate = 1.0,
		double start = 0, double end = -1) :
	CLerpAnimEffectInterval(std::to_string((unsigned long)this), 0, BT_no_blend) {
	    init(ctrl, rate, start, end);
	}
};

/* And now for something completely different:
 * A function to assist in loading animations, but also of other
 * general use.
 */

#include <pandaNode.h>
// Traverse the tree depth-first, left-to-right.  Assumes only one
// parent per node.  When "from" is non-NULL, continue search at its
// sibling, then move up and right from its parent, etc.
// No attempts are made to detect loops, or accomodate multiple parents.
PandaNode *find_node_type(const PandaNode *root, const TypeHandle &t,
                          const PandaNode *from = nullptr);