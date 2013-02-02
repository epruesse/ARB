// ================================================================ //
//                                                                  //
//   File      : cache.h                                            //
//   Purpose   : Cache for SmartPtrs                                //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef CACHE_H
#define CACHE_H

#ifndef SMARTPTR_H
#include "smartptr.h"
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif

#if defined(DEBUG)
#define DUMP_CACHE_STAT
#endif

#define ca_assert(cond)           arb_assert(cond)
#define ca_assert_expensive(cond) // arb_assert(cond) // uncomment to enable expensive assertions

// unittests for Cache are in ../PROBE/PT_io.cxx@TEST_CachedPtr

namespace cache {
    template <typename SMARTPTR> class Cache;
    template <typename SMARTPTR> class CacheHandle;

    template <typename SMARTPTR> class CacheEntry {
        typedef CacheEntry<SMARTPTR>* EntryPtr;
        typedef typename std::list< CacheEntry<SMARTPTR> >::iterator EntryIter;

        static EntryPtr& no_handle() {
            static EntryPtr none = NULL;
            return none;
        }

        EntryPtr& my_handle;
        SMARTPTR  data;
        EntryIter iter;

        CacheEntry(EntryPtr& handle)
            : my_handle(handle)
        {
        }
        CacheEntry(EntryPtr& handle, SMARTPTR& init_data)
            : my_handle(handle),
              data(init_data)
        {
        }
        void setClientPtr() { my_handle = this; }
        void clearClientPtr() { my_handle = NULL; }
        void setData(SMARTPTR& new_data) { data = new_data; }

        EntryIter getIterator() {
            ca_assert(data.isSet()); // otherwise iterator may be invalid
            return iter;
        }
        void setIterator(EntryIter newIter) { iter = newIter; }

        friend class Cache<SMARTPTR>;

    public:
        ~CacheEntry() { ca_assert(!my_handle); } // release before destruction
        SMARTPTR get_data() const { return data; }
    };


    /*! @class Cache
     *  @brief Cache for any SmartPtr type
     */
    template <typename SMARTPTR> class Cache {
        typedef CacheEntry<SMARTPTR> Entry;
        typedef Entry*               EntryPtr;
        typedef std::list<Entry>     Entries;

        typedef typename Entries::iterator EntryIter;

        Entries cached;

        size_t curr_size; // always == cached.size()
        size_t max_size;

#if defined(DUMP_CACHE_STAT)
        size_t insert_count;
        size_t access_count;
#endif

#if defined(ASSERTION_USED)
        bool curr_size_valid() const { return cached.size() == curr_size; }
#endif

        void keep(size_t kept_elems) {
            ca_assert(kept_elems <= max_size);
            size_t count = entries();
            if (count>kept_elems) {
                size_t    toRemove = count-kept_elems;
                EntryIter e        = cached.begin();

                while (toRemove--) e++->clearClientPtr();
                cached.erase(cached.begin(), e);
                curr_size = kept_elems;
                ca_assert_expensive(curr_size_valid());
                ca_assert(entries() == kept_elems);
            }
        }

        EntryIter top() { return --cached.end(); }

#if defined(ASSERTION_USED)
        bool is_member(EntryIter iter) {
            for (EntryIter i = cached.begin(); i != cached.end(); ++i) {
                if (i == iter) return true;
            }
            return false;
        }
        static bool is_valid_size(size_t Size) { return Size>0; }
#endif

        void insert(EntryPtr& my_handle, SMARTPTR& data) {
            ca_assert(data.isSet());
            if (my_handle) { // re-assign
                my_handle->setData(data);
                touch(*my_handle);
#if defined(DUMP_CACHE_STAT)
                --access_count; // do not count above touch as access
#endif
            }
            else { // initialization
                cached.push_back(CacheEntry<SMARTPTR>(my_handle, data));
                ++curr_size;
                ca_assert_expensive(curr_size_valid());

                cached.back().setClientPtr();
                ca_assert(&cached.back() == my_handle);

                my_handle->setIterator(top());

                keep(max_size);
                ca_assert(my_handle);
            }
#if defined(DUMP_CACHE_STAT)
            ++insert_count;
#endif
        }
        void touch(Entry& entry) {
            EntryIter iter = entry.getIterator();
            ca_assert_expensive(is_member(iter));
            if (iter != top()) { // not LRU entry
                cached.splice(cached.end(), cached, iter);
                ca_assert_expensive(is_member(iter));
                ca_assert(iter == top());
            }
#if defined(DUMP_CACHE_STAT)
            ++access_count;
#endif
        }
        void remove(EntryPtr& my_handle) {
            ca_assert(my_handle);
            EntryIter iter = my_handle->getIterator();
            ca_assert_expensive(is_member(iter));
            my_handle->clearClientPtr();
            cached.erase(iter);
            --curr_size;
            ca_assert_expensive(curr_size_valid());
            ca_assert(!my_handle);
        }

        friend class CacheHandle<SMARTPTR>;

    public:

        /*! create a new Cache
         *
         * @param Size specifies how many entries the Cache will hold at a time.
         * Can be redefined later using resize().
         *
         * Each cache entry is a SmartPtr (any flavour) which is represented by a CacheHandle.
         */
        Cache(size_t Size) : curr_size(0), max_size(Size) {
            ca_assert(is_valid_size(Size));
#if defined(DUMP_CACHE_STAT)
            access_count = 0;
            insert_count = 0;
#endif
        }
        /*! destroy the Cache
         *
         * Will invalidate all @ref CacheHandle "CacheHandles" assigned to this Cache.
         *
         */
        ~Cache() {
#if defined(DUMP_CACHE_STAT)
            printf("Cache-Stat: max_size=%zu inserts=%zu accesses=%zu\n", size(), insert_count, access_count);
#endif
            flush();
        }

        //! @return the number of currently cached CacheHandles
        size_t entries() const {
            ca_assert_expensive(curr_size_valid());
            return curr_size;
        }
        //! @return the maximum number of cached CacheHandles
        size_t size() const { return max_size; }
        /*! change the size of the Cache.
         *
         * @param new_size specifies the number of cache entries (as in constructor)
         */
        void resize(size_t new_size) {
            ca_assert(is_valid_size(new_size));
            if (new_size<max_size) keep(new_size);
            max_size = new_size;
        }
        //! flush the cache, i.e. uncache all elements
        void flush() { keep(0); }
    };

    /*! @class CacheHandle
     *  @brief Handle for Cache entries.
     */
    template <typename SMARTPTR> class CacheHandle : virtual Noncopyable {
        CacheEntry<SMARTPTR> *entry;
    public:
        /*! create a new, unbound CacheHandle
         *
         * Each (potential) cache entry needs a CacheHandle on client side.
         */
        CacheHandle() : entry(NULL) {}

        /*! destroys a CacheHandle
         *
         * Before destrying it, you need to either call release() or Cache::flush().
         */
        ~CacheHandle() { ca_assert(!is_cached()); }

        /*! assign data to a CacheHandle
         *
         * @param data SmartPtr containing client data (may not be NULL)
         * @param cache Cache in which data shall be stored
         *
         * The assigned data is stored in the Cache.
         *
         * When the cache is filled, calls to assign() will invalidate other
         * cache entries and their CacheHandles.
         */
        void assign(SMARTPTR data, Cache<SMARTPTR>& cache) {
            ca_assert(data.isSet()); // use release() to set a CacheHandle to NULL
            cache.insert(entry, data);
            ca_assert(is_cached());
        }
        /*! read data from cache
         *
         *  @param cache Cache to which data has been assigned earlier
         *  @return SmartPtr containing data (as used in assign())
         *
         *  You need to check whether the assigned data still is cached by calling is_cached()
         *  before you can use this function. e.g. like follows
         *
         *  @code
         *  if (!handle.is_cached()) handle.assign(generateYourData(), cache);
         *  smartptr = handle.access(cache);
         *  @endcode
         *
         *  The data behind the returned smartptr will stay valid until its destruction,
         *  even if the handle is invalidated by other code.
         *
         *  Calling access() will also make this CacheHandle the "newest" handle in cache,
         *  i.e. the one with the longest lifetime.
         */
        SMARTPTR access(Cache<SMARTPTR>& cache) {
            ca_assert(entry); // you did not check whether entry still is_cached()
            cache.touch(*entry);
            return entry->get_data();
        }
        /*! actively release data from cache
         *
         *  @param cache Cache to which data has been assigned earlier
         *
         * Call this either
         * - before destroying the CacheHandle or
         * - when you know the data will no longer be needed, to safe cache-space for other entries.
         */
        void release(Cache<SMARTPTR>& cache) {
            if (entry) {
                cache.remove(entry);
                ca_assert(!entry); // should be NULLed by cache.remove
                ca_assert(!is_cached());
            }
        }
        /*! check status of CacheHandle
         * @return true if CacheHandle (still) contains data
         */
        bool is_cached() const { return entry; }
    };

};

#else
#error cache.h included twice
#endif // CACHE_H
