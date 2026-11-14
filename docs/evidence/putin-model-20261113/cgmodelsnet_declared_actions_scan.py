"""Map exhaustive frame metrics to actions declared by the actual ANI files."""
import hashlib
import json
import re
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
result={}
for bank in ('man','mushketer'):
    metrics=json.loads((here/f'cgmodelsnet-neck-allframes-{bank}.json').read_text());rows=metrics['rows']
    data=(here/f'{bank}-source.ani').read_bytes();text=data.decode('cp1251',errors='replace')
    match=re.search(r'(?im)^\s*animation\s*=\s*([^\s;]+)',text);assert match and match.group(1).lower()==bank+'.an'
    parts=re.split(r'(?m)^\s*\[([^\]]+)\]',text);actions=[]
    for name,body in zip(parts[1::2],parts[2::2]):
        a=re.search(r'(?im)^\s*start_time\s*=\s*(-?\d+)',body);b=re.search(r'(?im)^\s*end_time\s*=\s*(-?\d+)',body)
        if not a or not b:continue
        start,end=int(a.group(1)),int(b.group(1));lo,hi=sorted((start,end))
        assert 0<=lo<=hi<len(rows),(bank,name,lo,hi)
        worst=max(rows[lo:hi+1],key=lambda row:row[1]);actions.append({'name':name,'start':start,'end':end,'worst_frame':worst[0],'max_edge':worst[1]})
    actions.sort(key=lambda r:r['max_edge'],reverse=True)
    print(bank,'declared actions',len(actions),'top',actions[:6])
    result[bank]={'ani_sha256':hashlib.sha256(data).hexdigest(),'bank_sha256':metrics['bank_sha256'],'model_sha256':metrics['candidate_sha256'],'actions':actions}
(here/'cgmodelsnet-declared-actions-scan.json').write_text(json.dumps(result,indent=2)+'\n')
