"""Prüft Sichern und Zurückspielen der Senderliste im Webportal: Dateiformat, Syntax- und Vollständigkeitsprüfung und Ablauf, ohne Gerät."""
import json
import re
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
START = '    async function saveAndRefresh() {'
END = '\n    // Sender-Modal schließen'

HARNESS = r'''
const vm=require('node:vm'),assert=require('node:assert/strict');
const source=SOURCE;
function make(){
 const log={alerts:[],confirms:[],saved:[],loads:0,links:[],blobs:[]};
 const context=vm.createContext({console,TextEncoder,TextDecoder,JSON,Object,Array,Set,Number,String,Math,Error,TypeError,Promise,setTimeout,
  t:k=>k,tf:(k,...a)=>[k,...a].join('|'),allStationsRemote:[],confirmAnswer:true,postOK:true,
  alert:m=>log.alerts.push(m),confirm:m=>{log.confirms.push(m);return context.confirmAnswer;},
  loadAllStationsRemote:async()=>{log.loads++;context.allStationsRemote=context.deviceStations;},
  loadPresets:async()=>{},ScaleWidget:{updateStations(){}},
  Blob:class{constructor(parts,o){this.text=parts.join('');this.type=o.type;log.blobs.push(this);}},
  URL:{createObjectURL:b=>'blob:1',revokeObjectURL(){}},
  document:{createElement:()=>{const l={click(){log.links.push({download:l.download,href:l.href});}};return l;}},
  fetch:async(url,options={})=>{
   assert.equal(url,'/api/stations');
   if(!options.method)return context.reply;
   assert.equal(options.method,'POST');assert.equal(options.headers['Content-Type'],'application/json');
   const list=JSON.parse(options.body);assert.ok(Array.isArray(list));log.saved.push(list);
   return {ok:context.postOK};
  }});
 vm.runInContext(source,context);
 return {context,log};
}
const file=(text,name='sender.json')=>{const bytes=typeof text==='string'?new TextEncoder().encode(text):text;
 return {name,size:bytes.length,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.length)};};
function stations(){
 const list=[];
 for(let i=0;i<70;i++)list.push(i%7===6?{shortName:'',name:'',url:'',preset:false}:
  {shortName:'Sender '+i,name:'Rundfunk "Ä\\'+i+'" 🎵',url:'https://radio.example/'+i+'?a=1&b=ä',preset:i<10});
 return list;
}
async function restore(text){const r=make();await r.context.restoreStationsFromFile(file(text));return r;}
async function rejected(text,...expected){
 const r=await restore(text);
 assert.equal(r.log.saved.length,0,'gespeichert trotz Fehler');assert.equal(r.log.loads,0);assert.equal(r.log.confirms.length,0);
 assert.equal(r.log.alerts.length,1);assert.ok(r.log.alerts[0].startsWith('js.stationRestoreHeader\n\n'),r.log.alerts[0]);
 for(const e of expected)assert.ok(r.log.alerts[0].includes('• '+e),'erwartet '+e+' in\n'+r.log.alerts[0]);
 return r.log.alerts[0];
}
const backup=list=>make().context.stationBackupText(list);
const edit=(fn,list=stations())=>{const d=JSON.parse(backup(list));fn(d);return JSON.stringify(d,null,1);};
(async()=>{
 const list=stations(),text=backup(list),lines=text.split('\n');
 // Format: Kopf, je Platz eine Zeile in Platzreihenfolge, gültiges JSON, Rundlauf ohne Verlust
 assert.equal(lines[1],'  "format": "RetroRadioTab5-Sender",');assert.equal(lines[2],'  "version": 1,');
 for(let i=0;i<70;i++)assert.ok(lines[4+i].startsWith('    {"slot":'+(i+1)+',"shortName":'),lines[4+i]);
 assert.equal(make().context.jsonSyntaxError(text),null);
 let r=await restore(text);
 assert.equal(r.log.loads,0);assert.deepEqual(r.log.saved,[list]);assert.deepEqual(r.log.alerts,['js.stationRestoreDone']);
 assert.deepEqual(r.log.confirms,['js.stationRestoreConfirm|sender.json|60|9']);
 // Nach fehlgeschlagenem POST den tatsächlichen Gerätestand neu laden
 r=make();r.context.postOK=false;r.context.deviceStations=list.slice(0,3);await r.context.restoreStationsFromFile(file(text));
 assert.equal(r.log.saved.length,1);assert.equal(r.log.loads,1);assert.deepEqual(r.log.alerts,['js.stationSaveError']);assert.deepEqual(r.context.allStationsRemote,list.slice(0,3));
 // Reihenfolge der Einträge ist egal, es zählt slot; BOM und andere Einrückung stören nicht
 r=await restore(edit(d=>d.stations.reverse()));assert.deepEqual(r.log.saved,[list]);
 r=make();await r.context.restoreStationsFromFile(file(new Uint8Array([0xEF,0xBB,0xBF,...new TextEncoder().encode(text)])));assert.deepEqual(r.log.saved,[list]);
 // Abgelehnte Rückfrage speichert nichts
 r=make();r.context.confirmAnswer=false;await r.context.restoreStationsFromFile(file(text));assert.equal(r.log.saved.length,0);assert.equal(r.log.loads,0);
 // Datei und Kodierung
 r=make();await r.context.restoreStationsFromFile({name:'x',size:65537,arrayBuffer:async()=>{throw new Error('nie lesen');}});
 assert.deepEqual(r.log.alerts,['js.stationRestoreHeader\n\n• js.stationRestoreTooBig']);
 r=make();await r.context.restoreStationsFromFile({name:'x',size:10,arrayBuffer:async()=>{throw new Error('weg');}});
 assert.deepEqual(r.log.alerts,['js.stationRestoreHeader\n\n• js.stationRestoreRead']);
 await rejected(new Uint8Array([0x7B,0x22,0xE4,0x22,0x7D]),'js.stationRestoreEncoding');
 await rejected('','js.stationRestoreEmpty');await rejected(' \n\t','js.stationRestoreEmpty');
 // Syntax mit Zeile und Spalte
 await rejected(lines.map((l,i)=>i===4?l.slice(0,-1):l).join('\n'),'js.stationRestoreSyntax|6|5|js.jsonError.syntax|');
 await rejected(text.replace('\n  ]',',\n  ]'),'js.stationRestoreSyntax|75|3|js.jsonError.syntax|');
 await rejected(text.slice(0,500),'js.stationRestoreSyntax|');
 assert.ok((await rejected(text.slice(0,500))).includes('|js.jsonError.end|'));
 const twice=text.replace('"slot":3,','"slot":3,"url":"http://x",'),row=twice.split('\n')[6];
 await rejected(twice,'js.stationRestoreSyntax|7|'+(row.indexOf('"url"',row.indexOf('"url"')+1)+1)+'|js.jsonError.duplicate|url');
 const escaped=text.replace('"slot":3,','"slot":3,"s\\u006cot":5,');
 await rejected(escaped,'js.stationRestoreSyntax|7|15|js.jsonError.duplicate|slot');
 await rejected(lines.map((l,i)=>i===4?l.slice(0,-1):l).join('\r'),'js.stationRestoreSyntax|6|5|js.jsonError.syntax|');
 await rejected(text+'x','js.stationRestoreSyntax|77|1|js.jsonError.trailing|');
 await rejected("{'format':1}",'js.stationRestoreSyntax|1|2|js.jsonError.syntax|');
 const comment='{"format":"RetroRadioTab5-Sender", // Kommentar\n}';
 await rejected(comment,'js.stationRestoreSyntax|1|'+(comment.indexOf('//')+1)+'|js.jsonError.syntax|');
 await rejected('{"a":"Zeile\numbruch"}','js.stationRestoreSyntax|1|12|js.jsonError.syntax|');
 await rejected('['.repeat(40),'js.stationRestoreSyntax|1|18|js.jsonError.depth|');
 // Der Prüfer stimmt mit JSON.parse überein
 const scan=make().context.jsonSyntaxError;
 for(const ok of ['0','-0.5e+3','"\\u00e4\\n\\"\\\\\\/"','[]','{}',' [1, {"a": [true, false, null]}] ','{"a":1,"b":{"a":2}}'])
  {assert.equal(scan(ok),null,ok);JSON.parse(ok);}
 for(const bad of ['01','-','1.','.5','+1','"\\x"','"\\u12g4"','[1,]','{"a":1,}','{"a" 1}','tru','nul','NaN','[1 2]','"\t"','{"a":1}{',' []'])
  {assert.notEqual(scan(bad),null,bad);assert.throws(()=>JSON.parse(bad),bad);}
 // Kennung, Version und Liste
 await rejected('[]','js.stationRestoreNotBackup');await rejected('{"format":"RetroRadio"}','js.stationRestoreNotBackup');
 await rejected(edit(d=>d.version=2),'js.stationRestoreNewer|2');
 await rejected(edit(d=>d.version='1'),'js.stationRestoreVersion');
 await rejected(edit(d=>{d.station=d.stations;delete d.stations;}),'js.stationRestoreUnknownTop|station','js.stationRestoreList');
 await rejected(edit(d=>d.stations.pop()),'js.stationRestoreCount|69','js.stationRestoreSlotMissing|70');
 await rejected(edit(d=>d.stations.push({...d.stations[0]})),'js.stationRestoreCount|71','js.stationRestoreSlotTwice|1');
 await rejected(edit(d=>d.stations[4]=null),'js.stationRestoreEntry|5','js.stationRestoreSlotMissing|5');
 await rejected(edit(d=>d.stations[3].slot=4.5),'js.stationRestoreSlot|js.stationRestoreEntryLabel|4');
 await rejected(edit(d=>d.stations[3].slot='4'),'js.stationRestoreSlot|js.stationRestoreEntryLabel|4');
 await rejected(edit(d=>d.stations[69].slot=71),'js.stationRestoreSlot|js.stationRestoreEntryLabel|70');
 await rejected(edit(d=>d.stations[2].slot=2),'js.stationRestoreSlotTwice|2','js.stationRestoreSlotMissing|3');
 // Felder je Platz
 const P='js.stationRestoreSlotLabel|';
 await rejected(edit(d=>delete d.stations[4].url),'js.stationRestoreField|'+P+'5|url');
 await rejected(edit(d=>delete d.stations[4].preset),'js.stationRestoreField|'+P+'5|preset');
 await rejected(edit(d=>{d.stations[1].shortname=d.stations[1].shortName;delete d.stations[1].shortName;}),'js.stationRestoreUnknown|'+P+'2|shortname','js.stationRestoreField|'+P+'2|shortName');
 await rejected(edit(d=>d.stations[0].name=5),'js.stationRestoreText|'+P+'1|name');
 await rejected(edit(d=>d.stations[0].preset='true'),'js.stationRestoreBool|'+P+'1');
 await rejected(edit(d=>d.stations[0].name='a\nb'),'js.stationRestoreChars|'+P+'1|name');
 await rejected(text.replace('"shortName":"Sender 0"','"shortName":"\\ud800"'),'js.stationRestoreChars|'+P+'1|shortName');
 await rejected(edit(d=>d.stations[0].shortName='ä'.repeat(12)),'js.stationRestoreLong|'+P+'1|shortName|24|23');
 await rejected(edit(d=>d.stations[0].name='x'.repeat(64)),'js.stationRestoreLong|'+P+'1|name|64|63');
 await rejected(edit(d=>d.stations[0].url='http://'+'x'.repeat(249)),'js.stationRestoreLong|'+P+'1|url|256|255');
 r=await restore(edit(d=>{d.stations[0].shortName='a'.repeat(23);d.stations[0].name='ü'.repeat(31)+'x';d.stations[0].url='http://'+'x'.repeat(248);}));assert.equal(r.log.saved.length,1);
 for(const url of ['ftp://radio','HTTP://radio','http://radio x','http:/radio','radio.example'])
  await rejected(edit(d=>d.stations[0].url=url),'js.stationRestoreUrl|'+P+'1');
 await rejected(edit(d=>d.stations[0].name=''),'js.stationRestoreNames|'+P+'1');
 await rejected(edit(d=>d.stations[6].name='Rest'),'js.stationRestoreEmptySlot|'+P+'7');
 await rejected(edit(d=>d.stations[6].preset=true),'js.stationRestoreEmptySlot|'+P+'7');
 await rejected(edit(d=>{d.stations[10].preset=true;d.stations[11].preset=true;}),'js.stationRestoreFavorites|11');
 // Viele Fehler: zehn gezeigt, Rest gezählt
 const many=await rejected(edit(d=>d.stations.forEach(s=>s.preset='x')));
 assert.equal(many.split('\n• ').length-1,10);assert.ok(many.endsWith('\njs.stationRestoreMore|60'));
 // RetroRadio nimmt ein reines Array auch oberhalb der Tab5-Grenze für den HTTP-Body entgegen
 const large=edit(d=>d.stations.forEach(s=>{if(s.url){s.url='http://'+'"'.repeat(248);s.name='"'.repeat(63);}}));
 assert.ok(file(large).size<=65536);r=await restore(large);assert.equal(r.log.saved.length,1);assert.ok(JSON.stringify(r.log.saved[0]).length>32768);
 // Sichern: frische Liste vom Gerät, Dateiname mit Datum, Inhalt wie stationBackupText
 r=make();r.context.reply={ok:true,json:async()=>list};
 await r.context.backupStations();
 assert.equal(r.log.links.length,1);assert.match(r.log.links[0].download,/^retroradio-sender_\d{4}-\d{2}-\d{2}\.json$/);
 assert.equal(r.log.blobs[0].type,'application/json');assert.equal(r.log.blobs[0].text,text);assert.equal(r.log.alerts.length,0);
 r=make();r.context.reply={ok:true,json:async()=>list.slice(0,2)};await r.context.backupStations();
 const padded=JSON.parse(r.log.blobs[0].text).stations;assert.equal(padded.length,70);assert.deepEqual(padded[69],{slot:70,shortName:'',name:'',url:'',preset:false});
 r=make();r.context.reply={ok:false,status:401,json:async()=>({success:false,error:'PIN erforderlich.'})};
 await r.context.backupStations();assert.deepEqual(r.log.alerts,['js.stationBackupError: PIN erforderlich.']);assert.equal(r.log.links.length,0);
})().catch(e=>{console.error(e);process.exit(1);});
'''


class SenderSicherungTest(unittest.TestCase):
    def setUp(self):
        self.html = (ROOT / 'webui/index.html').read_text(encoding='utf-8')
        self.source = self.html[self.html.index(START):self.html.index(END)]

    def test_format_pruefung_und_ablauf(self):
        """Rundlauf ohne Verlust, Zuordnung über slot, Syntaxfehler mit Zeile und Spalte, jede Feld- und Vollständigkeitsregel und kein Speichern bei Fehlern."""
        node = shutil.which('node')
        if not node:
            self.skipTest('Node ist nicht verfügbar.')
        with tempfile.TemporaryDirectory() as temporary:
            script = Path(temporary) / 'sicherung.cjs'
            script.write_text(HARNESS.replace('SOURCE', json.dumps(self.source)), encoding='utf-8')
            subprocess.run([node, str(script)], check=True)

    def test_tasten_und_texte(self):
        """Beide Tasten samt Dateiauswahl sind angebunden, jeder verwendete Text steht genau einmal auf Deutsch und einmal auf Englisch."""
        for element in ('id="btnStationBackup"', 'id="btnStationRestore"', 'id="stationRestoreInput"'):
            self.assertEqual(self.html.count(element), 1, element)
        self.assertIn("getElementById('btnStationBackup').addEventListener('click', backupStations)", self.html)
        keys = set(re.findall(r"\btf?\('([\w.]+\w)'", self.source))
        keys |= {f'js.jsonError.{kind}' for kind in ('syntax', 'end', 'duplicate', 'trailing', 'depth')}
        keys |= {'remote.stationBackup', 'remote.stationBackupTitle', 'remote.stationRestore', 'remote.stationRestoreTitle'}
        self.assertGreater(len(keys), 30)
        for key in sorted(keys):
            self.assertEqual(self.html.count(f"'{key}':"), 2, key)


if __name__ == '__main__':
    unittest.main()
