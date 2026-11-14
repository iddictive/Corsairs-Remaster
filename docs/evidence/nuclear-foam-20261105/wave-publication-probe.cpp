#include <atomic>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>
struct Frame{uint64_t generation;std::vector<uint64_t> data;};
int main(){std::shared_ptr<const Frame> published;std::atomic<bool> done{false};std::atomic<int> mismatches{0};std::vector<std::thread> readers;for(int i=0;i<4;i++)readers.emplace_back([&]{while(!done.load()){auto p=std::atomic_load(&published);if(!p)continue;for(auto v:p->data)if(v!=p->generation)mismatches.fetch_add(1);}});for(uint64_t i=1;i<=10000;i++){auto p=std::make_shared<Frame>();p->generation=i;p->data.assign(64,i);std::atomic_store(&published,std::shared_ptr<const Frame>(p));if(i%100==0)std::atomic_store(&published,std::shared_ptr<const Frame>());}done=true;for(auto&t:readers)t.join();printf("publications=10000 readers=4 resets=100 mismatches=%d\n",mismatches.load());return mismatches.load()!=0;}
