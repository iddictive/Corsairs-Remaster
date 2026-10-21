import urllib.request
import urllib.parse
import re
import sys

query = sys.argv[1] if len(sys.argv) > 1 else 'Корсары Возвращение легенды'
url = 'https://lite.duckduckgo.com/lite/'
data = urllib.parse.urlencode({'q': query}).encode('utf-8')
req = urllib.request.Request(url, data=data, headers={'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)'})
try:
    html = urllib.request.urlopen(req, timeout=10).read().decode('utf-8', errors='ignore')
    links = re.findall(r'href="(https?://[^"]+)"', html)
    for l in links[:20]:
        if 'duckduckgo' not in l:
            print(l)
except Exception as e:
    print('Error:', e)
