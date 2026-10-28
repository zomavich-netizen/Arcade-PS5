// Bake the approved CSS cabinet as a native, transparent OpenGL asset.
const fs=require('node:fs');
const {chromium}=require('playwright');
const root=require('node:path').resolve(__dirname,'..');
(async()=>{
const browser=await chromium.launch({headless:true});const p=await browser.newPage({viewport:{width:1200,height:1200},deviceScaleFactor:3,colorScheme:'dark'});
await p.setContent(fs.readFileSync(root+'/design/cabinet-source.html','utf8'));
await p.addStyleTag({content:`html,body{margin:0;padding:0;background:transparent}#arcade-directions .arcade{background:transparent;min-height:0;width:286px;height:450px;overflow:visible}#arcade-directions .vault-fixed{position:relative;width:286px;height:450px;filter:none}#arcade-directions .vault-game-track,#arcade-directions .pro-credit,#arcade-directions .vault-stick{visibility:hidden}#arcade-directions .pro-action span{position:absolute;inset:0;display:grid;place-items:center;width:auto;height:auto}#arcade-directions .pro-action svg{display:block;width:12px;height:12px;overflow:visible}`});
// Geometric symbols share a centered viewBox, avoiding font baseline offsets.
await p.locator('.pro-action span').evaluateAll(nodes=>{
 const paths=['<path d="M4 4L20 20M20 4L4 20"/>','<circle cx="12" cy="12" r="9"/>','<rect x="4" y="4" width="16" height="16" rx="1"/>','<path d="M12 3L22 21H2Z"/>'];
 nodes.forEach((n,i)=>{n.innerHTML='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">'+paths[i]+'</svg>';});
});
await p.locator('.pro-cabinet img').evaluateAll(nodes=>Promise.all(nodes.map(n=>n.decode())));const base=await p.locator('.pro-cabinet').boundingBox();
let layout={};for(const [name,selector] of Object.entries({screen:'.vault-screen-window',stick:'.vault-stick',start:'.pro-yellow-start',coin:'.pro-coin',credit:'.pro-credit'})){const r=await p.locator(selector).boundingBox();layout[name]={x:r.x-base.x,y:r.y-base.y,w:r.width,h:r.height}}
layout.actions=[];for(const n of await p.locator('.pro-action').all()){const r=await n.boundingBox();layout.actions.push({x:r.x-base.x,y:r.y-base.y,w:r.width,h:r.height})}
await p.locator('.pro-cabinet').screenshot({path:root+'/assets/vault-cabinet.png',omitBackground:true});fs.writeFileSync(root+'/design/cabinet-layout.json',JSON.stringify(layout,null,2)+'\n');
let header='#pragma once\n#include "platform/types.hpp"\nnamespace arcade::native::vault {\n';for(const [name,r]of Object.entries(layout)){if(name==='actions')continue;header+='constexpr Rect '+name+'{'+[r.x,r.y,r.w,r.h].map(v=>Number(v).toFixed(4)+'f').join(',')+'};\n'}header+='constexpr Rect actions[]={'+layout.actions.map(r=>'{'+[r.x,r.y,r.w,r.h].map(v=>Number(v).toFixed(4)+'f').join(',')+'}').join(',')+'};\n}\n';fs.writeFileSync(root+'/native/gfx/vault_layout.hpp',header);
await browser.close();console.log('Baked approved cabinet, native screen/control geometry and credit label bounds.');
})().catch(e=>{console.error(e);process.exit(1)});
