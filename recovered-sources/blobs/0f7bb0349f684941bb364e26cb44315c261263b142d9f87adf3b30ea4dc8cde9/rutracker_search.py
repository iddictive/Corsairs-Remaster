import urllib.request
import urllib.parse
import re

url = 'https://rutracker.org/forum/tracker.php'
data = urllib.parse.urlencode({'nm': 'Возвращение Легенды'}).encode('windows-1251')
req = urllib.request.Request(url, data=data, headers={'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36'})
html = urllib.request.urlopen(req).read().decode('windows-1251', errors='ignore')
matches = re.findall(r'<a[^>]+href="viewtopic.php?t=([0-9]+)"[^>]*>(.*?)</a>', html)
for m in matches:
    print(m[0], ':', m[1])
