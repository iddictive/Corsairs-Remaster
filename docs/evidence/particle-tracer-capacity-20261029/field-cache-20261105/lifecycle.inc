#include "system/data_source/data_bool.h"
#include "system/data_source/data_string.h"
#include "system/data_desc/data_desc.h"
#include "vma.hpp"
#include "string_compare.hpp"
#include <cassert>
struct ProbeDesc : DataDescripion { void CreateFields() override {} };
static void verifyFields(FieldList &list) {
  const char *names[] = {"ax", "AX", "bh", "absent", "new field"};
  for (const char *name : names) {
    for (auto type : {FIELD_BOOL, FIELD_STRING, FIELD_GRAPH}) {
      void *expected=nullptr;
      for (uint32_t i=0; i<list.GetFieldCount(); ++i) {
        const auto &f=list.GetFieldByIndex(i);
        if (f.Type==type && f.HashValue==MakeHashValue(name) && storm::iEquals(f.Name,name)) { expected=f.pPointer; break; }
      }
      void *actual = type==FIELD_BOOL ? static_cast<void*>(list.FindBool(name)) : type==FIELD_STRING ? static_cast<void*>(list.FindString(name)) : static_cast<void*>(list.FindGraph(name));
      if(actual!=expected) std::abort();
    }
  }
}
static void lifecycleProbe() {
  if(MakeHashValue("ax")!=MakeHashValue("bh")) std::abort();
  MemFile file; file.OpenWrite(4096); uint32_t count=4; file.WriteType(count);
  FieldType type=FIELD_BOOL; DataBool b;
  b.SetName("ax"); b.SetValue(true); file.WriteType(type); b.Write(&file);
  b.SetName("bh"); b.SetValue(false); file.WriteType(type); b.Write(&file);
  b.SetName("ax"); b.SetValue(false); file.WriteType(type); b.Write(&file);
  type=FIELD_STRING; DataString text; text.SetName("ax"); text.SetValue("original"); file.WriteType(type); text.Write(&file);
  FieldList list; file.Seek(0,VFSEEK_SET); list.Load(&file); verifyFields(list);
  BillboardFields cache; cache.Refresh(list); auto revision = list.GetRevision();
  if (!cache.initialized || cache.revision != revision) std::abort();
  if(!list.GetBool("AX") || list.GetBool("bh") || strcmp(list.GetString("ax"),"original")) std::abort();
  list.FindBool("ax")->SetValue(false); list.FindString("ax")->SetValue("edited"); verifyFields(list);
  if(list.GetRevision()!=revision || cache.revision!=revision) std::abort();
  if(list.GetBool("ax") || strcmp(list.GetString("ax"),"edited")) std::abort();
  file.Seek(0,VFSEEK_SET); list.Load(&file); verifyFields(list);
  if(list.GetRevision()==revision) std::abort(); cache.Refresh(list); revision=list.GetRevision();
  ProbeDesc desc; desc.Clear(); desc.AddField(FIELD_BOOL,"bh"); desc.AddField(FIELD_GRAPH,"new field"); list.Convert(&desc); verifyFields(list);
  if(!list.FindGraph("new field") || list.FindBool("ax") || list.GetRevision()==revision) std::abort();
  cache.Refresh(list); revision=list.GetRevision();
  list.DelAll(); verifyFields(list);
  if(list.GetRevision()==revision) std::abort(); cache.Refresh(list);
  file.Seek(0,VFSEEK_SET); list.Load(&file); verifyFields(list);
  if(!list.GetBool("ax")) std::abort();
  list.DelAll();
  ProbeDesc live; live.Clear(); live.AddField(FIELD_BOOL, PARTICLE_DIR_ORIENT); live.AddField(FIELD_STRING, ATTACHEDEMITTER_NAME);
  list.Convert(&live); cache.Refresh(list);
  cache.speedOriented->SetValue(true); cache.attachedEmitter->SetValue("edited");
  if(!cache.speedOriented->GetValue() || strcmp(cache.attachedEmitter->GetValue(),"edited")) std::abort();
  revision=list.GetRevision(); list.DelAll(); list.Convert(&live);
  if(list.GetRevision()==revision) std::abort(); cache.Refresh(list);
  if(cache.speedOriented!=list.FindBool(PARTICLE_DIR_ORIENT) || cache.attachedEmitter!=list.FindString(ATTACHEDEMITTER_NAME)) std::abort();
  list.DelAll(); puts("field lifecycle parity: passed");
}
