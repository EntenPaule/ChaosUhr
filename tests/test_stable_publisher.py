"""Offline safety/ordering/idempotency tests. Pushes only to a temporary local bare repo."""
from pathlib import Path
import copy,hashlib,importlib.util,json,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('publisher',ROOT/'scripts/publish_stable_channel.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)

class PublisherTests(unittest.TestCase):
 def fixtures(self):
  files={};assets=[];checks=[]
  for name,size in [('chaosuhr-2.0.11-esp8266.bin',5000),('chaosuhr-2.0.11-esp32-ota.bin',6000)]:
   data=b'\xe92.0.11\0'+b'x'*(size-8);url=f'https://github.com/{p.REPO}/releases/download/v2.0.11/{name}';files[url]=data;assets.append(dict(name=name,size=size,browser_download_url=url,digest='sha256:'+p.sha(data)));checks.append(p.sha(data)+'  '+name)
  url=f'https://github.com/{p.REPO}/releases/download/v2.0.11/SHA256SUMS.txt';files[url]=('\n'.join(checks)+'\n').encode();assets.append(dict(name='SHA256SUMS.txt',size=len(files[url]),browser_download_url=url))
  release=dict(id=1,tag_name='v2.0.11',draft=False,prerelease=False,assets=assets)
  download=lambda url:(files[url],{})
  return release,files,download
 def test_plan_and_rejections(self):
  r,files,download=self.fixtures();source=(ROOT/'include/display_options.h').read_text()
  plan=p.prepare(r,r,source,download);self.assertEqual(plan['stable.json']['manifest']['supported_led_gpio'],[4,5,12,13,14]);self.assertEqual(plan['stable-esp32.json']['manifest']['led_gpio'],32)
  for mutation in ['draft','prerelease','missing','factory','size','digest','sha']:
   bad=copy.deepcopy(r)
   if mutation in ['draft','prerelease']:bad[mutation]=True
   elif mutation=='missing':bad['assets'].pop()
   elif mutation=='factory':bad['assets'][0]['name']='chaosuhr-2.0.11-esp32-factory.bin'
   elif mutation=='size':bad['assets'][0]['size']+=1
   elif mutation=='digest':bad['assets'][0]['digest']='sha256:'+'0'*64
   else:
    key=bad['assets'][-1]['browser_download_url'];files[key]=files[key].replace(files[key][:64],b'0'*64)
   with self.subTest(mutation=mutation),self.assertRaises(RuntimeError):p.prepare(bad,r,source,download)
  with self.assertRaises(RuntimeError):p.prepare(r,{**r,'tag_name':'v2.0.12'},source,download)
  for tag in ['v2.0.11-beta.3','v2.0.11;echo secret','v02.0.11','2.0.11']:
   with self.assertRaises(RuntimeError):p.version(tag)
 def test_explicit_approval_before_network(self):
  with tempfile.TemporaryDirectory() as temp:
   result=subprocess.run(['python3',str(ROOT/'scripts/publish_stable_channel.py'),'--tag','v2.0.11','--report',temp+'/report.json'],capture_output=True,text=True)
   self.assertNotEqual(result.returncode,0);self.assertIn('authorization',result.stdout);self.assertFalse(json.loads(Path(temp+'/report.json').read_text())['complete'])
 def test_publish_order_interrupt_idempotence_and_preview(self):
  r,files,download=self.fixtures();plan=p.prepare(r,r,(ROOT/'include/display_options.h').read_text(),download)
  with tempfile.TemporaryDirectory() as temp:
   temp=Path(temp);remote=temp/'remote.git';channel=temp/'channel'
   p.run(['git','init','--bare',remote]);p.run(['git','init',channel]);p.run(['git','config','user.name','Offline Test'],channel);p.run(['git','config','user.email','test@example.invalid'],channel);p.run(['git','remote','add','origin',remote],channel)
   for name in ['stable.json','stable-esp32.json']:(channel/name).write_text(json.dumps({'version':'2.0.10'}))
   for name in ['preview.json','preview-esp32.json']:(channel/name).write_bytes(b'preview must stay byte-identical\n')
   (channel/'old.bin').write_bytes(b'old');p.run(['git','add','.'],channel);p.run(['git','commit','-m','baseline'],channel);p.run(['git','push','origin','HEAD:refs/heads/firmware-updates'],channel)
   def unavailable(url,data):raise RuntimeError('simulated public failure')
   with self.assertRaises(RuntimeError):p.publish(channel,plan,unavailable)
   self.assertEqual(json.loads((channel/'stable.json').read_text())['version'],'2.0.10')
   self.assertEqual(p.run(['git','rev-list','--count','HEAD'],channel),'2') # binary commit only
   events=[]
   def verify(url,data):
    events.append(url)
    if url.endswith('.bin'):self.assertEqual(json.loads((channel/'stable.json').read_text())['version'],'2.0.10')
   first=p.publish(channel,plan,verify)
   self.assertTrue(events[0].endswith('.bin') and events[1].endswith('.bin'));self.assertTrue(events[2].endswith('.json'))
   head=p.run(['git','rev-parse','HEAD'],channel);self.assertEqual(p.run(['git','rev-list','--count','HEAD'],channel),'3')
   p.publish(channel,plan,lambda u,d:None);self.assertEqual(p.run(['git','rev-parse','HEAD'],channel),head)
   self.assertEqual((channel/'preview.json').read_bytes(),b'preview must stay byte-identical\n');self.assertEqual((channel/'old.bin').read_bytes(),b'old')
   (channel/'stable.json').write_text(json.dumps({'version':'2.0.12'}));p.run(['git','add','.'],channel);p.run(['git','commit','-m','newer stable'],channel)
   with self.assertRaises(RuntimeError):p.publish(channel,plan,lambda u,d:None)

if __name__=='__main__':unittest.main()
