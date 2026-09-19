"use strict";
//jshint browser: true, esversion: 6, varstmt: true

const readPsiData=(data,proc,startSec,ctx)=>{
  data=new DataView(data);
  ctx=ctx||{};
  if(!ctx.pids){
    ctx.pids=[];
    ctx.dict=[];
    ctx.pos=0;
    ctx.trailerSize=0;
    ctx.timeListCount=-1;
    ctx.codeListPos=0;
    ctx.codeCount=0;
    ctx.initTime=-1;
    ctx.currTime=-1;
  }
  while(data.byteLength-ctx.pos>=ctx.trailerSize+32){
    let pos=ctx.pos+ctx.trailerSize;
    const timeListLen=data.getUint16(pos+10,true);
    const dictionaryLen=data.getUint16(pos+12,true);
    const dictionaryWindowLen=data.getUint16(pos+14,true);
    const dictionaryDataSize=data.getUint32(pos+16,true);
    const dictionaryBuffSize=data.getUint32(pos+20,true);
    const codeListLen=data.getUint32(pos+24,true);
    if(data.getUint32(pos)!=0x50737363||
       data.getUint32(pos+4)!=0x0d0a9a0a||
       dictionaryWindowLen<dictionaryLen||
       dictionaryBuffSize<dictionaryDataSize||
       dictionaryWindowLen>65536-4096){
      return null;
    }
    const chunkSize=32+timeListLen*4+dictionaryLen*2+Math.ceil(dictionaryDataSize/2)*2+codeListLen*2;
    if(data.byteLength-pos<chunkSize)break;
    let timeListPos=pos+32;
    pos+=32+timeListLen*4;
    if(ctx.timeListCount<0){
      const pids=[];
      const dict=[];
      let sectionListPos=0;
      for(let i=0;i<dictionaryLen;i++,pos+=2){
        const codeOrSize=data.getUint16(pos,true)-4096;
        if(codeOrSize>=0){
          if(codeOrSize>=ctx.pids.length||ctx.pids[codeOrSize]<0)return null;
          pids[i]=ctx.pids[codeOrSize];
          dict[i]=ctx.dict[codeOrSize];
          ctx.pids[codeOrSize]=-1;
        }else{
          pids[i]=codeOrSize;
          dict[i]=null;
          sectionListPos+=2;
        }
      }
      sectionListPos+=pos;
      for(let i=0;i<dictionaryLen;i++){
        if(pids[i]>=0)continue;
        dict[i]=new Uint8Array(data.buffer.slice(sectionListPos,sectionListPos+pids[i]+4097));
        sectionListPos+=pids[i]+4097;
        pids[i]=data.getUint16(pos,true)&0x1fff;
        pos+=2;
      }
      for(let i=dictionaryLen,j=0;i<dictionaryWindowLen;j++){
        if(j>=ctx.pids.length)return null;
        if(ctx.pids[j]<0)continue;
        pids[i]=ctx.pids[j];
        dict[i++]=ctx.dict[j];
      }
      ctx.pids=pids;
      ctx.dict=dict;
      ctx.timeListCount=0;
      pos=sectionListPos+dictionaryDataSize%2;
    }else{
      pos+=dictionaryLen*2+Math.ceil(dictionaryDataSize/2)*2;
    }
    pos+=ctx.codeListPos;
    timeListPos+=ctx.timeListCount*4;
    for(;ctx.timeListCount<timeListLen;ctx.timeListCount++,timeListPos+=4){
      let initTime=ctx.initTime;
      let currTime=ctx.currTime;
      const absTime=data.getUint32(timeListPos,true);
      if(absTime==0xffffffff){
        currTime=-1;
      }else if(absTime>=0x80000000){
        currTime=absTime&0x3fffffff;
        if(initTime<0)initTime=currTime;
      }else{
        const n=data.getUint16(timeListPos+2,true)+1;
        if(currTime>=0){
          currTime+=data.getUint16(timeListPos,true);
          const sec=((currTime+0x40000000-initTime)&0x3fffffff)/11250;
          if(sec>=(startSec||0)){
            for(;ctx.codeCount<n;ctx.codeCount++,pos+=2,ctx.codeListPos+=2){
              const code=data.getUint16(pos,true)-4096;
              if(!proc(sec,ctx.dict,code,ctx.pids[code]))return false;
            }
            ctx.codeCount=0;
          }else{
            pos+=n*2;
            ctx.codeListPos+=n*2;
          }
        }else{
          pos+=n*2;
          ctx.codeListPos+=n*2;
        }
      }
      ctx.initTime=initTime;
      ctx.currTime=currTime;
    }
    ctx.pos=pos;
    ctx.trailerSize=2+(2+chunkSize)%4;
    ctx.timeListCount=-1;
    ctx.codeListPos=0;
    ctx.currTime=-1;
  }
  const ret=data.buffer.slice(ctx.pos);
  ctx.pos=0;
  return ret;
};

const progressPsiDataChatMixedStream=(reader,onData,onChat)=>{
  return new Promise((resolve,reject)=>{
    let readCount=0;
    let response="";
    const ctx={};
    let atobRemain="";
    let psiData=new Uint8Array(0);
    const decoder=new TextDecoder();
    const readNext=()=>{
      reader.read().then(r=>{
        if(r.done){
          if(readCount)resolve(readCount);
          else reject("Error: Empty stream");
          return;
        }
        response+=decoder.decode(r.value,{stream:true});
        let offset=0;
        while(offset<response.length){
          let i=response.indexOf("<",offset);
          if(i==offset){
            i=response.indexOf("\n",offset);
            if(i<0)break;
            if(onChat)onChat(response.substring(offset,i));
            offset=i+1;
          }else{
            i=i<0?response.length:i;
            const n=Math.floor((i-offset+atobRemain.length)/4)*4;
            if(n){
              const addData=base64ToUint8Array(atobRemain+response.substring(offset,offset+n-atobRemain.length));
              atobRemain=response.substring(offset+n-atobRemain.length,i);
              const concatData=new Uint8Array(psiData.length+addData.length);
              concatData.set(psiData);
              concatData.set(addData,psiData.length);
              psiData=readPsiData(concatData.buffer,(sec,dict,code,pid)=>{
                if(onData)onData(pid,dict,code,Math.floor(sec*90000));
                return true;
              },0,ctx);
              if(psiData)psiData=new Uint8Array(psiData);
            }else{
              atobRemain+=response.substring(offset,i);
            }
            offset=i;
          }
        }
        readCount+=offset;
        response=response.substring(offset);
        readNext();
      }).catch(()=>{
        if(readCount)resolve(readCount);
        else reject("Error: Empty stream");
      });
    };
    readNext();
  });
};

const decodeB24CaptionFromCueText=(text,work)=>{
  work=work||[];
  text=text.replace(/\r?\n/g,"");
  const re=/<v b24caption[0-8]>(.*?)<\/v>/g;
  let src,ret=null;
  while((src=re.exec(text))!==null){
    src=src[1].replace(/<.*?>/g,"").replace(/&(?:amp|lt|gt|quot|apos);/g,
      m=>m=="&amp;"?"&":m=="&lt;"?"<":m=="&gt;"?">":m=="&quot;"?'"':"'");
    const brace=[];
    let wl=0,hi=0;
    for(let i=0;i<src.length;){
      if(src[i]=="%"){
        if((++i)+2>src.length)return null;
        const c=src[i++];
        const d=src[i++];
        if(c=="^"){
          work[wl++]=0xc2;
          work[wl++]=d.charCodeAt(0)+64;
        }else if(c=="="){
          if(d=="{"){
            work[wl++]=0;
            work[wl++]=0;
            work[wl++]=0;
            brace.push(wl);
          }else if(d=="}"&&brace.length>0){
            const pos=brace.pop();
            work[pos-3]=wl-pos>>16&255;
            work[pos-2]=wl-pos>>8&255;
            work[pos-1]=wl-pos&255;
          }else return null;
        }else if(c=="+"){
          if(d=="{"){
            const pos=src.indexOf("%+}",i);
            if(pos<0)return null;
            try{
              const buf=atob(src.substring(i,pos));
              for(let j=0;j<buf.length;j++)work[wl++]=buf.charCodeAt(j);
            }catch(e){return null;}
            i=pos+3;
          }else return null;
        }else{
          const x=c.charCodeAt(0);
          const y=d.charCodeAt(0);
          work[wl++]=(x>=97?x-87:x>=65?x-55:x-48)<<4|(y>=97?y-87:y>=65?y-55:y-48);
        }
      }else{
        let x=src.charCodeAt(i++);
        if(x<0x80){
          work[wl++]=x;
        }else if(x<0x800){
          work[wl++]=0xc0|x>>6;
          work[wl++]=0x80|x&63;
        }else if(0xd800<=x&&x<=0xdbff){
          hi=x;
        }else if(0xdc00<=x&&x<=0xdfff){
          x=0x10000+((hi&0x3ff)<<10)+(x&0x3ff);
          work[wl++]=0xf0|x>>18;
          work[wl++]=0x80|x>>12&63;
          work[wl++]=0x80|x>>6&63;
          work[wl++]=0x80|x&63;
        }else{
          work[wl++]=0xe0|x>>12;
          work[wl++]=0x80|x>>6&63;
          work[wl++]=0x80|x&63;
        }
      }
    }
    if(brace.length>0)return null;
    if(3<=wl&&wl<=65520){
      const r=new Uint8Array(wl+7);
      r[0]=0x80;
      r[1]=0xff;
      r[2]=0xf0;
      r[3]=work[0];
      r[4]=work[1];
      r[5]=work[2];
      r[6]=wl-3>>8&255;
      r[7]=wl-3&255;
      for(let i=3;i<wl;i++)r[i+5]=work[i];
      ret=ret||[];
      ret.push(r);
    }
  }
  return ret;
};

const waitForHlsStart=(src,postQuery,interval,delay)=>{
  return new Promise((resolve,reject)=>{
    let options={
      method:"POST",
      headers:{"Content-Type":"application/x-www-form-urlencoded"},
      body:postQuery
    };
    const poll=()=>{
      fetch(src,options).then(response=>{
        if(response.ok)return response.text();
        reject();
      }).then(text=>{
        if(text.indexOf("#EXT-X-MEDIA-SEQUENCE:")<0)setTimeout(poll,interval);
        else setTimeout(()=>{resolve(src);},delay);
      }).catch(()=>{reject();});
    };
    poll();
    options=null;
  });
};

const unescapeHtml=s=>{
  return s.replace(/&(?:amp|lt|gt|quot|apos|#10|#13);/g,
    m=>m[1]=="l"?"<":m[1]=="g"?">":m[1]=="q"?'"':m[3]=="p"?"&":m[3]=="o"?"'":m[3]=="0"?"\n":"\r");
};

const chatTagColors={
  red:"#ff0000",
  pink:"#ff8080",
  orange:"#ffc000",
  yellow:"#ffff00",
  green:"#00ff00",
  cyan:"#00ffff",
  blue:"#0000ff",
  purple:"#c000ff",
  black:"#000000",
  white2:"#cccc99",
  niconicowhite:"#cccc99",
  red2:"#cc0033",
  truered:"#cc0033",
  pink2:"#ff33cc",
  orange2:"#ff6600",
  passionorange:"#ff6600",
  yellow2:"#999900",
  madyellow:"#999900",
  green2:"#00cc66",
  elementalgreen:"#00cc66",
  cyan2:"#00cccc",
  blue2:"#3399ff",
  marineblue:"#3399ff",
  purple2:"#6633cc",
  nobleviolet:"#6633cc",
  black2:"#666666"
};

const getChatTagColorRe=new RegExp("(?:^| )(#[0-9A-Fa-f]{6}|"+Object.keys(chatTagColors).join("|")+")(?: |$)");

const parseChatTag=tag=>{
  let m=tag.match(/^<chat(?= )(.*)>(.*?)<\/chat>$/);
  if(m){
    const a=m[1];
    const r={text:unescapeHtml(m[2])};
    m=a.match(/ date="(\d+)"/);
    if(m){
      r.date=parseInt(m[1],10);
      if(r.date>=0){
        m=a.match(/ mail="(.*?)"/);
        r.mail=m?m[1]:"";
        m=r.mail.match(/(?:^| )(ue|shita)(?: |$)/);
        r.type=!m?"right":m[1]=="ue"?"top":"bottom";
        m=r.mail.match(getChatTagColorRe);
        r.colorcode=!m?"#ffffff":m[1][0]=="#"?m[1]:chatTagColors[m[1]];
        r.color=parseInt(r.colorcode.substring(1),16);
        r.yourpost=/ yourpost="1"/.test(a);
        r.refuge=/ nx_jikkyo="1"| x_refuge="1"/.test(a);
        m=a.match(/ user_id="([0-9A-Za-z_:-]*)"/);
        r.user=m?m[1]:"";
        return r;
      }
    }
  }
  return null;
};

const readJikkyoLog=(text,proc,startSec,ctx)=>{
  ctx=ctx||{};
  if(ctx.pos===undefined){
    ctx.pos=0;
    ctx.currSec=-1;
  }
  for(;;){
    const i=text.indexOf("\n",ctx.pos);
    if(i<0)break;
    const tag=text.substring(ctx.pos,i);
    let sec=ctx.currSec;
    if(tag.startsWith("<!-- J="))sec++;
    if(sec>=(startSec||0)&&!proc(sec,tag))break;
    ctx.pos=i+1;
    ctx.currSec=sec;
  }
};

const getJikkyoLogStats=(text)=>{
  let sec=0;
  for(let pos=0;;){
    const i=text.indexOf("\n",pos);
    if(i<0)break;
    if(text.startsWith("<!-- J=",pos))++sec;
    pos=i+1;
  }
  const windowSec=Math.max(Math.floor(sec/400),5);
  sec=0;
  let counts=[0],maxCount=0;
  for(let pos=0;;){
    const i=text.indexOf("\n",pos);
    if(i<0)break;
    if(text.startsWith("<!-- J=",pos)){
      if(++sec%windowSec==0){
        maxCount=Math.max(counts[counts.length-1],maxCount);
        counts.push(0);
      }
    }else if(text.startsWith("<chat ",pos)){
      counts[counts.length-1]++;
    }
    pos=i+1;
  }
  return {sec,windowSec,counts,maxCount};
};

const drawStatsGraph=(stats,now,ofs)=>{
  if(!stats||!(stats.maxCount>0))return;
  const w=stats.counts.length;
  const h=50;
  now/=stats.sec;
  now=Math.floor((now>0?Math.min(now,1):0)*w);
  ofs/=stats.sec;
  ofs=Math.floor((ofs>0?Math.min(ofs,1):ofs<0?Math.max(ofs,-1):0)*w);
  if(!stats.canvas){
    stats.canvas=document.createElement("canvas");
    stats.canvas.width=w;
    stats.canvas.height=h;
  }else if(stats.now==now&&stats.ofs==ofs){
    //No redraw required
    return;
  }
  stats.now=now;
  stats.ofs=ofs;
  const ctx=stats.canvas.getContext("2d");
  ctx.clearRect(0,0,w,h);
  ctx.fillStyle="#888";
  if(ofs>0)ctx.fillRect(0,0,ofs,h);
  else if(ofs<0)ctx.fillRect(w+ofs,0,w,h);
  ctx.strokeStyle="#07d";
  ctx.lineWidth=1;
  for(let x=0;x<w;x++){
    ctx.beginPath();
    ctx.moveTo(x+0.5,h);
    ctx.lineTo(x+0.5,h-Math.floor(stats.counts[x]/stats.maxCount*h));
    ctx.closePath();
    ctx.stroke();
  }
  ctx.strokeStyle="#f00";
  ctx.lineWidth=2;
  ctx.beginPath();
  ctx.moveTo(now,0);
  ctx.lineTo(now,h);
  ctx.closePath();
  ctx.stroke();
};

//Global variables available after runOnscreenButtonsScript() is called.
let vid,vcont,vfull,vwrap,setCheckLivePosition,setMinimizeJikkyo,setSendComment,hideOnscreenButtons;

//Seek the video to `sec` position if possible.
let seekVideo=(sec)=>{};

const adjustVideoMaxWidth=()=>{
  if(!vwrap.style.width){
    const r=(vid.e.clientWidth>0&&vid.e.clientHeight>0?vid.e.clientWidth/vid.e.clientHeight:16/9)*window.innerHeight/window.innerWidth-
            document.getElementById("footer").clientHeight*1.5/window.innerHeight;
    vcont.style.setProperty("--vcont-max-width",(r<0.5?0.5:r<1?r:1)*100+"%");
  }
  if(vid.c){
    //Workaround to preserve canvas aspect ratio...
    const r=(vid.e.width>0&&vid.e.height>0?vid.e.width/vid.e.height:16/9)*window.innerHeight/window.innerWidth;
    if(r<1)vfull.classList.add("video-full-container-flex-row");
    else vfull.classList.remove("video-full-container-flex-row");
  }
};

const runPlaybackScript=(isTrusted)=>{
  vid={e:document.getElementById("video"),unmute(){(vid.c||vid.e).muted=false;}};
  vid.initSrc=vid.e.dataset.src||vid.e.getAttribute("src");
  if(vid.e.tagName=="CANVAS"){
    //Behave like HTMLMediaElement.
    vid.currentTime=0;
    vid.muted=false;
    vid.volume=1;
    vid.c=vid;
  }
  const btnUnmute=document.getElementById("vid-unmute");
  if(btnUnmute.dataset.initialMuted&&(!isTrusted||btnUnmute.dataset.initialMuted!="auto")){
    (vid.c||vid.e).muted=true;
  }
  if(btnUnmute.dataset.initialVolume){
    (vid.c||vid.e).volume=+btnUnmute.dataset.initialVolume;
  }
  if((vid.c||vid.e).muted){
    btnUnmute.style.display=null;
    btnUnmute.onclick=()=>{
      vid.unmute();
      btnUnmute.style.display="none";
    };
  }
  const cbDatacast=document.getElementById("cb-datacast");
  if(cbDatacast){
    const prefix="nvram_prefix=receiverinfo%2F";
    if(cbDatacast.dataset.absentZip&&!localStorage.getItem(prefix+"zipcode")){
      localStorage.setItem(prefix+"zipcode",btoa(cbDatacast.dataset.absentZip));
    }
    if(cbDatacast.dataset.absentPrefecture&&!localStorage.getItem(prefix+"prefecture")){
      localStorage.setItem(prefix+"prefecture",btoa(String.fromCharCode(cbDatacast.dataset.absentPrefecture)));
    }
    if(cbDatacast.dataset.absentRegion&&!localStorage.getItem(prefix+"regioncode")){
      localStorage.setItem(prefix+"regioncode",btoa(String.fromCharCode(cbDatacast.dataset.absentRegion>>8,cbDatacast.dataset.absentRegion&0xff)));
    }
  }
  vcont=document.getElementById("vid-cont");
  vfull=document.getElementById("vid-full");
  vwrap=document.getElementById("vid-wrap");
  window.addEventListener("load",adjustVideoMaxWidth);
  window.addEventListener("my-load",adjustVideoMaxWidth);
  window.addEventListener("resize",adjustVideoMaxWidth);
  const createButton=(t,f)=>{
    const btn=document.createElement("button");
    btn.type="button";
    btn.innerText=t;
    btn.onclick=e=>{
      e.stopPropagation();
      f();
    };
    return btn;
  };
  const vseek=document.getElementById("vid-seek");
  const bfull=document.createElement("div");
  bfull.className="full-control";
  bfull.appendChild(createButton("full",()=>{(vfull.requestFullscreen||vfull.webkitRequestFullscreen||vfull.webkitRequestFullScreen).call(vfull);}));
  let pip=null;
  if(window.documentPictureInPicture){
    bfull.appendChild(document.createElement("br"));
    bfull.appendChild(createButton("PiP",()=>{
      documentPictureInPicture.requestWindow({
        width:vcont.clientWidth/2,
        height:vcont.clientHeight/1.9
      }).then(pipWindow=>{
        pip=pipWindow;
        for(const styleSheet of document.styleSheets){
          let css="";
          for(const rule of styleSheet.cssRules){css+=rule.cssText;}
          const style=document.createElement("style");
          style.textContent=css;
          pipWindow.document.head.appendChild(style);
        }
        vcont.classList.add("video-container-pip");
        if(cbDatacast)cbDatacast.disabled=true;
        pip.document.body.appendChild(vcont);
        if(vseek)vcont.appendChild(vseek);
        pip.onpagehide=()=>{
          if(vseek){
            const vseekMarker=document.getElementById("vid-seek-marker");
            vseekMarker.parentNode.insertBefore(vseek,vseekMarker);
          }
          vfull.insertBefore(vcont,vfull.firstChild);
          if(cbDatacast)cbDatacast.disabled=false;
          vcont.classList.remove("video-container-pip");
          pip=null;
        };
      });
    }));
  }
  const bexit=document.createElement("div");
  bexit.className="exit-control";
  bexit.appendChild(createButton("exit",()=>{(document.exitFullscreen||document.webkitExitFullscreen||document.webkitCancelFullScreen).call(document);}));
  const bpipx=document.createElement("div");
  bpipx.className="pip-exit-control";
  bpipx.appendChild(createButton("exit",()=>{if(pip)pip.close();}));
  if(document.fullscreenEnabled||document.webkitFullscreenEnabled){
    let showCursor=()=>{};
    if(vid.c){
      let t=0;
      showCursor=()=>{
        vfull.classList.remove("hide-cursor-on-fullscreen");
        clearTimeout(t);
        t=setTimeout(()=>{vfull.classList.add("hide-cursor-on-fullscreen");},3000);
      };
      vfull.onmousemove=showCursor;
    }
    vfull[document.fullscreenEnabled?"onfullscreenchange":"onwebkitfullscreenchange"]=()=>{
      showCursor();
      if(vseek){
        const vseekMarker=document.getElementById("vid-seek-marker");
        if(document.fullscreenElement||document.webkitFullscreenElement)vcont.appendChild(vseek);
        else vseekMarker.parentNode.insertBefore(vseek,vseekMarker);
      }
    };
  }
  const blive=document.createElement("div");
  blive.className="live-control";
  blive.style.display="none";
  blive.appendChild(createButton("\u2192",null));
  setCheckLivePosition=f=>{
    blive.style.display=null;
    vid.e.ondurationchange=f;
    setInterval(f,500);
    blive.firstElementChild.onclick=()=>{vid.e.currentTime=f();};
    setCheckLivePosition=null;
  };
  const bminjk=document.createElement("div");
  bminjk.className="minimize-jikkyo";
  bminjk.style.display="none";
  bminjk.appendChild(createButton("",null));
  setMinimizeJikkyo=f=>{
    bminjk.style.display=f?null:"none";
    bminjk.firstElementChild.onclick=f;
  };
  const bcomm=document.createElement("div");
  bcomm.className="comment-control";
  bcomm.style.display="none";
  bcomm.appendChild(createButton("\u2713",()=>{bcomm.classList.toggle("opaque");}));
  const commInput=document.createElement("input");
  commInput.type="text";
  commInput.className="nico";
  let commSend=null;
  commInput.onkeydown=e=>{
    if(!e.isComposing&&e.keyCode!=229&&e.key=="Enter"){
      if(commSend&&commInput.value)commSend(commInput);
      commInput.value="";
    }
  };
  bcomm.appendChild(commInput);
  bcomm.appendChild(createButton("\u226b",()=>{
    if(commSend&&commInput.value)commSend(commInput);
    commInput.value="";
  }));
  const bmove=document.createElement("div");
  bmove.className="move-comment-control";
  bmove.style.display="none";
  bmove.appendChild(createButton("\u00a0",()=>{
    bcomm.classList.toggle("moved");
    bmove.classList.toggle("moved");
  }));
  setSendComment=f=>{
    bcomm.style.display=f?null:"none";
    bmove.style.display=f?null:"none";
    commSend=f;
  };
  let hideButtonsTimer=0;
  for(const b of [bfull,bexit,bpipx]){
    b.onclick=()=>{
      vcont.classList.add("video-container-show-buttons");
      clearTimeout(hideButtonsTimer);
      hideButtonsTimer=setTimeout(()=>{vcont.classList.remove("video-container-show-buttons");},3000);
    };
  }
  let removed=true;
  hideOnscreenButtons=hide=>{
    const a=[bfull,bexit,bpipx,blive,bminjk,bcomm,bmove];
    if(!removed&&hide){
      for(const b of a){vcont.removeChild(b);}
      removed=true;
    }else if(removed&&!hide){
      for(const b of a){vcont.appendChild(b);}
      removed=false;
    }
  };
  hideOnscreenButtons(false);
  if(document.getElementById("cb-jikkyo")){
    runJikkyoScript();
  }
  if(vid.c){
    runTranscodeScript();
    runTsliveScript();
  }else if(vseek){
    runTranscodeScript();
    if(vid.e.dataset.hls)runHlsScript();
  }else{
    runVideoScript();
  }
};

//Global variables available after runJikkyoScript() is called.
let onJikkyoStream=null;
let onJikkyoStreamError=null;
let checkJikkyoDisplay=()=>{};
let addJikkyoMessage;
let toggleJikkyo;
let shiftJikkyo=()=>{};

const runJikkyoScript=()=>{
  let danmaku=null;
  const comm=document.getElementById("jikkyo-comm");
  if(comm.dataset.shiftable){
    for(const sec of [15,1,-1,-15]){
      const btn=document.createElement("button");
      btn.type="button";
      btn.innerText=(sec>0?"+":"")+sec;
      btn.onclick=()=>{shiftJikkyo(sec);};
      comm.insertBefore(btn,comm.firstChild);
    }
  }
  document.getElementById("jikkyo-config").previousElementSibling.onclick=()=>{
    document.getElementById("jikkyo-config").classList.toggle("display");
  };
  const chats=document.getElementById("jikkyo-chats");
  let checkScrollID=0;
  const cbJikkyo=document.getElementById("cb-jikkyo");
  let customReplace=[];
  try{
    customReplace=JSON.parse(decodeURIComponent(cbJikkyo.dataset.customReplaceJson));
    for(const rep of customReplace){
      rep.regex=new RegExp(rep.pattern,rep.flags);
    }
  }catch(e){
    console.warn("customReplaceJson:",e);
    customReplace=[];
  }
  const replaceTag=(tag)=>{
    for(const rep of customReplace){
      tag=tag.replace(rep.regex,rep.replace);
    }
    return tag;
  };
  const cbJikkyoOnscr=document.getElementById("cb-jikkyo-onscr");
  const onclickJikkyoOnscr=()=>{
    if(danmaku&&comm.style.visibility!="hidden"){
      const cbDatacast=document.getElementById("cb-datacast");
      if((!cbDatacast||!cbDatacast.checked)&&cbJikkyoOnscr.checked)danmaku.show();
      else danmaku.hide();
    }
  };
  cbJikkyoOnscr.onclick=onclickJikkyoOnscr;
  checkJikkyoDisplay=()=>{
    if(danmaku){
      if(!cbJikkyo.checked){
        danmaku.hide();
        comm.style.visibility="hidden";
        const cbDatacast=document.getElementById("cb-datacast");
        if(!cbDatacast||!cbDatacast.checked){
          comm.style.display="none";
        }
        setMinimizeJikkyo(null);
      }else{
        comm.style.visibility=null;
        comm.style.display=null;
        setMinimizeJikkyo(()=>{comm.classList.toggle("minimized");});
        onclickJikkyoOnscr();
      }
    }
  };
  addJikkyoMessage=text=>{
    const b=document.createElement("strong");
    b.innerText=text;
    const div=document.createElement("div");
    div.appendChild(b);
    chats.appendChild(div);
  };
  toggleJikkyo=enabled=>{
    clearInterval(checkScrollID);
    checkScrollID=0;
    if(!enabled){
      onJikkyoStream=null;
      onJikkyoStreamError=null;
      checkJikkyoDisplay();
      return;
    }
    if(!danmaku){
      danmaku=new Danmaku({
        container:vcont,
        opacity:1,
        callback(){},
        error(){},
        apiBackend:{read(opt){opt.success([]);}},
        height:+cbJikkyo.dataset.commentHeight,
        duration:+cbJikkyo.dataset.commentDuration,
        paddingTop:10,
        paddingBottom:10,
        unlimited:false,
        api:{id:"noid",address:"noad",token:"noto",user:"nous",speedRate:1}
      });
    }
    checkJikkyoDisplay();
    let commHide=true;
    checkScrollID=setInterval(()=>{
      if(getComputedStyle(comm).display=="none"||getComputedStyle(vfull).display=="none"){
        commHide=true;
      }else{
        const scroll=Math.abs(chats.scrollTop+chats.clientHeight-chats.scrollHeight)<chats.clientHeight/4;
        //The top/bottom border of #jikkyo-comm must be 1px
        comm.style.height=vid.e.clientHeight-2+"px";
        chats.style.height=vid.e.clientHeight-2+comm.getBoundingClientRect().y-chats.getBoundingClientRect().y+"px";
        if(commHide||scroll)chats.scrollTop=chats.scrollHeight;
        commHide=false;
      }
    },1000);
    let fragment=null;
    const scatter=[];
    let scatterInterval=200;
    let closed=false;
    let jkID="?";
    onJikkyoStream=tag=>{
      if(tag.startsWith("<chat ")){
        const c=parseChatTag(replaceTag(tag));
        if(c){
          if(c.yourpost)c.border="2px solid #c00";
          scatter.push(c);
          const dateSpan=document.createElement("span");
          dateSpan.innerText=String(100+(Math.floor(c.date/3600)+9)%24).substring(1)+":"+
                             String(100+Math.floor(c.date/60)%60).substring(1)+":"+
                             String(100+c.date%60).substring(1);
          const userSpan=document.createElement(c.yourpost?"b":"span");
          userSpan.innerText="("+c.user.substring(c.user.substring(0,2)=="a:"?2:0).substring(0,3)+")";
          userSpan.className=c.refuge?"refuge":"nico";
          const span=document.createElement("span");
          span.innerText=c.text;
          if(c.color!=0xffffff){
            span.style.backgroundColor=c.colorcode;
            span.className=(c.color>>16)*3+(c.color>>8)%256*6+c.color%256<255?"dark":"light";
          }
          const div=document.createElement("div");
          if(closed){
            div.className="closed";
            closed=false;
          }
          div.appendChild(dateSpan);
          div.appendChild(userSpan);
          div.appendChild(span);
          if(!fragment)fragment=document.createDocumentFragment();
          fragment.appendChild(div);
        }
        return;
      }else if(tag.startsWith("<chat_result ")){
        const m=tag.match(/^[^>]*? status="(\d+)"/);
        if(m&&m[1]!="0")addJikkyoMessage("Error! (chat_result="+m[1]+")");
        return;
      }else if(tag.startsWith("<x_room ")){
        const m=tag.match(/^[^>]*? nickname="(.*?)"/);
        const nickname=m?m[1]:"";
        const loggedIn=/^[^>]*? is_logged_in="1"/.test(tag);
        const refuge=/^[^>]*? refuge="1"/.test(tag);
        addJikkyoMessage("Connected to "+(refuge?"refuge":"nicovideo")+" jk"+jkID+" ("+(loggedIn?"login=":"")+nickname+")");
        return;
      }else if(tag.startsWith("<x_disconnect ")){
        const m=tag.match(/^[^>]*? status="(\d+)"/);
        const refuge=/^[^>]*? refuge="1"/.test(tag);
        if(m)addJikkyoMessage("Disconnected from "+(refuge?"refuge":"nicovideo")+" (status="+m[1]+")");
        return;
      }else if(tag.startsWith("<!-- M=")){
        if(tag.substring(7,22)=="Closed logfile.")closed=true;
        else if(tag.substring(7,31)!="Started reading logfile:")addJikkyoMessage(tag.substring(7,tag.length-4));
        return;
      }else if(!tag.startsWith("<!-- J=")){
        return;
      }
      jkID=tag.match(/^<!-- J=(\d*)/)[1]||"?";
      if(tag.indexOf(";T=")<0)scatterInterval=90;
      else scatterInterval=Math.min(Math.max(scatterInterval+(scatter.length>0?-10:10),100),200);
      setTimeout(()=>{
        const scroll=Math.abs(chats.scrollTop+chats.clientHeight-chats.scrollHeight)<chats.clientHeight/4;
        if(fragment){
          chats.appendChild(fragment);
          fragment=null;
        }
        if(scatterInterval<100){
          danmaku.draw(scatter);
          scatter.splice(0);
        }
        const n=Math.ceil(scatter.length/5);
        if(n>0){
          for(let i=0;i<5;i++){
            setTimeout(()=>{
              if(scatter.length>0){
                danmaku.draw(scatter.slice(0,n));
                scatter.splice(0,n);
              }
            },scatterInterval*i);
          }
        }
        if(commHide||scroll){
          while(chats.childElementCount>1000){
            chats.removeChild(chats.firstElementChild);
          }
        }
        if(scroll)chats.scrollTop=chats.scrollHeight;
      },0);
    };
    onJikkyoStreamError=addJikkyoMessage;
  };
};

const runVideoScript=()=>{
  const inputFile=document.getElementById("input-file");
  if(inputFile){
    inputFile.onchange=()=>{
      vid.e.src=URL.createObjectURL(inputFile.files[0]);
      inputFile.parentNode.parentNode.removeChild(inputFile.parentNode);
    };
  }
  const cbCaption=document.getElementById("cb-caption");
  const vidTrack=document.getElementById("vid-track");
  if(vidTrack&&vidTrack.kind=="metadata"){
    vidTrack.oncuechange=()=>{
      vidTrack.oncuechange=null;
      const work=[];
      const dataList=[];
      for(const cue of vidTrack.track.cues){
        const ret=decodeB24CaptionFromCueText(cue.text,work);
        if(!ret)return;
        for(const pes of ret){dataList.push({pts:cue.startTime,pes});}
      }
      let aribb24Option=null;
      try{
        aribb24Option=JSON.parse(decodeURIComponent(cbCaption.dataset.aribb24OptionJson));
      }catch(e){
        console.warn("aribb24OptionJson:",e);
      }
      let cap;
      try{
        cap=cbCaption.dataset.aribb24UseSvg?new aribb24js.SVGRenderer(aribb24Option):new aribb24js.CanvasRenderer(aribb24Option);
        cap.attachMedia(vid.e);
      }catch(e){
        console.warn("aribb24js:",e);
        return;
      }
      cbCaption.onclick=()=>{
        if(cbCaption.checked)cap.show();
        else cap.hide();
      };
      document.getElementById("label-caption").style.display="inline";
      if(!cbCaption.checked)cap.hide();
      dataList.reverse();
      const pushCap=()=>{
        for(let i=0;i<100;i++){
          const data=dataList.pop();
          if(!data)return;
          cap.pushRawData(data.pts,data.pes);
        }
        setTimeout(pushCap,0);
      };
      pushCap();
    };
  }else if(vidTrack){
    cbCaption.onclick=()=>{
      vidTrack.track.mode=cbCaption.checked?"showing":"hidden";
    };
    document.getElementById("label-caption").style.display="inline";
    if(cbCaption.checked)vidTrack.track.mode="showing";
  }
  const cbDatacast=document.getElementById("cb-datacast");
  if(cbDatacast){
    if(cbDatacast.parentNode.textContent=="data"){
      let onDataStream=null;
      let onDataStreamError=null;
      let reopen=false;
      let reconnectCount=0;
      let ctrl=null;
      const openSubStream=()=>{
        if(reopen)return;
        if(ctrl){
          ctrl.abort();
          ctrl=null;
          if(onDataStream){
            reopen=true;
            setTimeout(()=>{reopen=false;openSubStream();},5000);
          }
          return;
        }
        if(!onDataStream)return;
        ctrl=new AbortController();
        //TODO: follow ratechange
        fetch("xcode.lua?fname="+vid.initSrc.replace(/^(?:\.\.\/)+/,"")+"&psidata=1&ofssec="+Math.floor(vid.e.currentTime),{
          signal:ctrl.signal
        }).then(response=>{
          if(!response.ok)throw new Error(response.status+" "+response.statusText);
          return progressPsiDataChatMixedStream(response.body.getReader(),(...args)=>{if(ctrl&&onDataStream)onDataStream(...args);},null);
        }).then(readCount=>{
          if(ctrl){
            if(onDataStreamError)onDataStreamError("Done: "+readCount+" bytes");
            if(++reconnectCount>2)reconnectCount=0;
            else openSubStream();
            ctrl=null;
          }
        }).catch(e=>{
          if(ctrl){
            if(onDataStreamError)onDataStreamError(""+e);
            if(++reconnectCount>2)reconnectCount=0;
            else openSubStream();
            ctrl=null;
          }
        });
      };
      cbDatacast.checked=false;
      cbDatacast.onclick=()=>{
        document.querySelector(".remote-control").style.display=cbDatacast.checked?"":"none";
        if(!cbDatacast.checked){
          onDataStream=null;
          onDataStreamError=null;
          openSubStream();
          hideOnscreenButtons(false);
          bmlBrowserSetInvisible(true);
          vwrap.style.width=null;
          vwrap.style.height=null;
          checkJikkyoDisplay();
          return;
        }
        checkJikkyoDisplay();
        vwrap.style.width=vfull.clientWidth+"px";
        vwrap.style.height=vfull.clientHeight+"px";
        bmlBrowserSetVisibleSize(vcont.clientWidth,vcont.clientHeight);
        hideOnscreenButtons(true);
        bmlBrowserSetInvisible(false);
        onDataStream=(pid,dict,code,pcr)=>{
          dict[code]=bmlBrowserPlayTSSection(pid,dict[code],pcr)||dict[code];
        };
        onDataStreamError=text=>{
          document.querySelector(".remote-control-indicator").innerText=text;
        };
        openSubStream();
      };
      vid.e.onseeked=openSubStream;
    }else{
      let psiData=null;
      let readTimer=0;
      let videoLastSec=0;
      const startRead=()=>{
        clearTimeout(readTimer);
        const startSec=vid.e.currentTime;
        videoLastSec=startSec;
        const ctx={};
        const read=()=>{
          const videoSec=vid.e.currentTime;
          if(videoSec<videoLastSec||videoLastSec+10<videoSec){
            startRead();
            return;
          }
          videoLastSec=videoSec;
          if(psiData&&readPsiData(psiData,(sec,dict,code,pid)=>{
              dict[code]=bmlBrowserPlayTSSection(pid,dict[code],Math.floor(sec*90000))||dict[code];
              return sec<videoSec;
            },startSec,ctx)!==false){
            startRead();
            return;
          }
          readTimer=setTimeout(read,500);
        };
        readTimer=setTimeout(read,500);
      };
      let psiDataFetched=false;
      const cbDatacast=document.getElementById("cb-datacast");
      cbDatacast.checked=false;
      cbDatacast.onclick=()=>{
        document.querySelector(".remote-control").style.display=cbDatacast.checked?"":"none";
        if(!cbDatacast.checked){
          clearTimeout(readTimer);
          readTimer=0;
          hideOnscreenButtons(false);
          bmlBrowserSetInvisible(true);
          vwrap.style.width=null;
          vwrap.style.height=null;
          checkJikkyoDisplay();
          return;
        }
        startRead();
        checkJikkyoDisplay();
        vwrap.style.width=vfull.clientWidth+"px";
        vwrap.style.height=vfull.clientHeight+"px";
        bmlBrowserSetVisibleSize(vcont.clientWidth,vcont.clientHeight);
        hideOnscreenButtons(true);
        bmlBrowserSetInvisible(false);
        if(psiDataFetched)return;
        psiDataFetched=true;
        fetch(vid.initSrc.replace(/\.[0-9A-Za-z]+$/,"")+".psc").then(response=>{
          if(!response.ok)throw new Error(response.status+" "+response.statusText);
          return response.arrayBuffer();
        }).then(arrayBuffer=>{
          psiData=arrayBuffer;
        }).catch(e=>{
          document.querySelector(".remote-control-indicator").innerText=""+e;
          psiDataFetched=false;
        });
      };
    }
  }
  const cbJikkyo=document.getElementById("cb-jikkyo");
  if(cbJikkyo){
    let offsetSec=0;
    shiftJikkyo=sec=>{
      offsetSec+=sec;
      addJikkyoMessage("Offset "+offsetSec+"sec");
    };
    let logText=null;
    let readTimer=0;
    let videoLastSec=0;
    let stats=null;
    const startRead=()=>{
      clearTimeout(readTimer);
      const startSec=vid.e.currentTime+offsetSec;
      videoLastSec=startSec;
      const ctx={};
      const read=()=>{
        const videoSec=vid.e.currentTime+offsetSec;
        if(videoSec<videoLastSec||videoLastSec+10<videoSec){
          startRead();
          return;
        }
        videoLastSec=videoSec;
        if(logText){
          readJikkyoLog(logText,(sec,tag)=>{
            if(onJikkyoStream)onJikkyoStream(tag);
            return sec<videoSec;
          },startSec,ctx);
          drawStatsGraph(stats,videoSec,offsetSec);
        }
        readTimer=setTimeout(read,200);
      };
      readTimer=setTimeout(read,200);
    };
    const selectID=document.querySelector('#jikkyo-config > select[name="id"]');
    const inputTM=document.querySelector('#jikkyo-config > input[name="tm"]');
    const inputTMSec=document.querySelector('#jikkyo-config > select[name="tmsec"]');
    let jkID=0,jkTM=0;
    let logTextFetched=false;
    const onclickJikkyo=()=>{
      cbJikkyo.onclick=onclickJikkyo;
      if(!cbJikkyo.checked){
        toggleJikkyo(false);
        clearTimeout(readTimer);
        readTimer=0;
        return;
      }
      toggleJikkyo(true);
      startRead();
      if(logTextFetched)return;
      logTextFetched=true;
      fetch("jklog.lua?fname="+vid.initSrc.replace(/^(?:\.\.\/)+/,"")+"&fsec="+Math.floor(vid.e.duration)+"&jkid="+jkID+"&jktm="+jkTM).then(response=>{
        if(!response.ok)throw new Error(response.status+" "+response.statusText);
        return response.text();
      }).then(text=>{
        logText=text;
        const m=logText.match(/^<!-- J=([0-9]+);T=([0-9]+)/);
        if(m){
          for(const opt of selectID.options){
            if(opt.value==m[1]){
              opt.selected=true;
              break;
            }
          }
          inputTM.value=new Date(1000*m[2]+32400000).toISOString().substring(0,16);
          inputTMSec.options[m[2]%60].selected=true;
        }
        const comm=document.getElementById("jikkyo-comm");
        if(stats&&stats.canvas){
          comm.removeChild(stats.canvas);
        }
        stats=getJikkyoLogStats(logText);
        drawStatsGraph(stats);
        if(stats.canvas){
          comm.insertBefore(stats.canvas,comm.firstChild);
        }
      }).catch(e=>{
        if(onJikkyoStreamError)onJikkyoStreamError(""+e);
        logTextFetched=false;
      });
    };
    const checkStartTimer=setInterval(()=>{
      if(vid.e.duration<Infinity){
        clearInterval(checkStartTimer);
        const btnConfig=document.querySelector("#jikkyo-config > button");
        btnConfig.onclick=selectID.onchange=()=>{
          if(logTextFetched&&logText==null)return;
          jkID=+(selectID.value||"0");
          jkTM=inputTM.value?Math.floor(Date.parse(inputTM.value+"Z")/60000)*60+inputTMSec.selectedIndex-32400:0;
          logText=null;
          logTextFetched=false;
          onclickJikkyo();
        };
        onclickJikkyo();
      }
    },500);
  }
  seekVideo=(sec)=>{
    vid.e.currentTime=sec;
  };
  const vidChapters=document.getElementById("vid-chapters");
  if(vidChapters){
    vidChapters.selectedIndex=-1;
    vidChapters.onchange=()=>{
      if(vidChapters.selectedIndex>=0)seekVideo(+vidChapters.options[vidChapters.selectedIndex].value);
    };
  }
};

const runTranscodeScript=()=>{
  const vseek=document.getElementById("vid-seek");
  vid.ofssec=+vseek.dataset.initialOfssec;
  vid.fast=+vseek.dataset.initialFast;
  const cbLive=document.getElementById("cb-live");
  const postCommentQuery=cbLive&&cbLive.dataset.postCommentQuery;
  let currentAbsTime;
  if(vid.c){
    //Playback rate is controlled on client-side.
    currentAbsTime=()=>vid.ofssec+Math.floor(vid.c.currentTime);
  }else{
    currentAbsTime=()=>vid.ofssec+Math.floor(vid.e.currentTime*vid.fast);
    if(vid.e.controlsList)vid.e.controlsList.add("noplaybackrate");
    if(window.createMiscWasmModule){
      setTimeout(()=>{
        createMiscWasmModule().then(mod=>{
          //Functions for drawing thumbnails
          vid.getGrabberInputBuffer=mod.getGrabberInputBuffer;
          vid.grabFirstFrame=mod.grabFirstFrame;
        });
      },0);
    }
    const diffs=[0,0,0,0,0];
    let duration=-1;
    let lastseek=0;
    const checkLivePosition=()=>{
      let interval=Math.max(diffs[0],diffs[1],diffs[2],diffs[3],diffs[4])+1;
      let seekable=vid.e.duration;
      if(seekable==Infinity)seekable=vid.e.seekable.length>0?vid.e.seekable.end(vid.e.seekable.length-1):0;
      if(seekable>0){
        if(duration<0)duration=seekable;
        if(seekable-duration>0.5){
          diffs.shift();
          diffs.push(seekable-duration);
          duration=seekable;
          interval=Math.max(diffs[0],diffs[1],diffs[2],diffs[3],diffs[4])+1;
          if(vid.e.currentTime<duration-interval*2-3&&Date.now()-lastseek>10000){
            if(cbLive&&cbLive.checked){
              vid.e.currentTime=duration-interval;
              lastseek=Date.now();
            }
          }
        }
      }
      return duration-interval;
    };
    setCheckLivePosition(checkLivePosition);
  }
  const rangeSeek=document.querySelector("#vid-seek input");
  const adjustSeekbarWidth=()=>{
    let othersWidth=0;
    for(const e of document.querySelectorAll(".video-side-item")){othersWidth+=e.offsetWidth;}
    rangeSeek.style.width=Math.max(1-othersWidth/window.innerWidth,0.3)*100+"%";
  };
  window.addEventListener("load",adjustSeekbarWidth);
  window.addEventListener("my-load",adjustSeekbarWidth);
  window.addEventListener("resize",adjustSeekbarWidth);
  vid.fastParam="";
  let openSubStream=()=>{};
  const cbJikkyo=document.getElementById("cb-jikkyo");
  const selectID=document.querySelector('#jikkyo-config > select[name="id"]');
  const inputTM=document.querySelector('#jikkyo-config > input[name="tm"]');
  const inputTMSec=document.querySelector('#jikkyo-config > select[name="tmsec"]');
  const shiftable=cbJikkyo&&document.getElementById("jikkyo-comm").dataset.shiftable;
  if(shiftable){
    //Get all comments at once.
    let offsetSec=0;
    shiftJikkyo=sec=>{
      offsetSec+=sec;
      addJikkyoMessage("Offset "+offsetSec+"sec");
    };
    let logText=null;
    let readTimer=0;
    let videoLastSec=0;
    let stats=null;
    const startRead=()=>{
      clearTimeout(readTimer);
      const startSec=currentAbsTime()+offsetSec;
      videoLastSec=startSec;
      const ctx={};
      const read=()=>{
        const videoSec=currentAbsTime()+offsetSec;
        if(videoSec<videoLastSec||videoLastSec+10<videoSec){
          startRead();
          return;
        }
        videoLastSec=videoSec;
        if(logText){
          readJikkyoLog(logText,(sec,tag)=>{
            if(onJikkyoStream)onJikkyoStream(tag);
            return sec<videoSec;
          },startSec,ctx);
          drawStatsGraph(stats,videoSec,offsetSec);
        }
        readTimer=setTimeout(read,200);
      };
      readTimer=setTimeout(read,200);
    };
    let jkID=0,jkTM=0;
    let logTextFetched=false;
    const onclickJikkyo=()=>{
      cbJikkyo.onclick=onclickJikkyo;
      document.querySelector('#vid-form input[name="jikkyo"]').value=cbJikkyo.checked?"1":"0";
      if(!cbJikkyo.checked){
        toggleJikkyo(false);
        clearTimeout(readTimer);
        readTimer=0;
        return;
      }
      toggleJikkyo(true);
      startRead();
      if(logTextFetched)return;
      logTextFetched=true;
      fetch("jklog.lua"+vid.initSrc.match(/\?fname=[^&]*/)[0]+"&jkid="+jkID+"&jktm="+jkTM).then(response=>{
        if(!response.ok)throw new Error(response.status+" "+response.statusText);
        return response.text();
      }).then(text=>{
        logText=text;
        const m=logText.match(/^<!-- J=([0-9]+);T=([0-9]+)/);
        if(m){
          for(const opt of selectID.options){
            if(opt.value==m[1]){
              opt.selected=true;
              break;
            }
          }
          inputTM.value=new Date(1000*m[2]+32400000).toISOString().substring(0,16);
          inputTMSec.options[m[2]%60].selected=true;
        }
        const comm=document.getElementById("jikkyo-comm");
        if(stats&&stats.canvas){
          comm.removeChild(stats.canvas);
        }
        stats=getJikkyoLogStats(logText);
        drawStatsGraph(stats);
        if(stats.canvas){
          comm.insertBefore(stats.canvas,comm.firstChild);
        }
      }).catch(e=>{
        if(onJikkyoStreamError)onJikkyoStreamError(""+e);
        logTextFetched=false;
      });
    };
    const btnConfig=document.querySelector("#jikkyo-config > button");
    btnConfig.onclick=selectID.onchange=()=>{
      if(logTextFetched&&logText==null)return;
      jkID=+(selectID.value||"0");
      jkTM=inputTM.value?Math.floor(Date.parse(inputTM.value+"Z")/60000)*60+inputTMSec.selectedIndex-32400:0;
      logText=null;
      logTextFetched=false;
      onclickJikkyo();
    };
    setTimeout(onclickJikkyo,500);
  }
  const cbDatacast=document.getElementById("cb-datacast");
  if(cbDatacast||(cbJikkyo&&!shiftable)){
    let onDataStream=null;
    let onDataStreamError=null;
    let jkID=0,jkTM=0;
    {
      let reopen=false;
      let reconnectCount=0;
      let ctrl=null;
      openSubStream=()=>{
        if(reopen)return;
        if(ctrl){
          ctrl.abort();
          ctrl=null;
          if(onDataStream||(onJikkyoStream&&!shiftable)){
            reopen=true;
            setTimeout(()=>{reopen=false;openSubStream();},postCommentQuery?5000:2000);
          }
          return;
        }
        if(!onDataStream&&!(onJikkyoStream&&!shiftable))return;
        let mHeader=null;
        ctrl=new AbortController();
        fetch((vid.fastParam?vid.initSrc.replace(/&fast=[^&]*/,"")+vid.fastParam:vid.initSrc)+(onDataStream?"&psidata=1":"")+
              (onJikkyoStream&&!shiftable?"&jikkyo=1&jkid="+jkID+"&jktm="+jkTM:"")+"&ofssec="+currentAbsTime(),{
          signal:ctrl.signal
        }).then(response=>{
          if(!response.ok)throw new Error(response.status+" "+response.statusText);
          return progressPsiDataChatMixedStream(response.body.getReader(),(...args)=>{if(ctrl&&onDataStream)onDataStream(...args);},s=>{
            if(ctrl){
              if(!mHeader&&!!(mHeader=s.match(/^<!-- J=([0-9]+)(?:;T=([0-9]+))?/))){
                for(const opt of selectID.options){
                  if(opt.value==mHeader[1]){
                    opt.selected=true;
                    break;
                  }
                }
                if(mHeader[2]){
                  const tm=mHeader[2]-currentAbsTime();
                  inputTM.value=new Date(1000*tm+32400000).toISOString().substring(0,16);
                  inputTMSec.options[tm%60].selected=true;
                }
              }
              if(onJikkyoStream)onJikkyoStream(s);
            }
          });
        }).then(readCount=>{
          if(ctrl){
            if(onDataStreamError)onDataStreamError("Done: "+readCount+" bytes");
            if(onJikkyoStreamError&&!shiftable)onJikkyoStreamError("Done: "+readCount+" bytes");
            if(++reconnectCount>2)reconnectCount=0;
            else openSubStream();
            ctrl=null;
          }
        }).catch(e=>{
          if(ctrl){
            if(onDataStreamError)onDataStreamError(""+e);
            if(onJikkyoStreamError&&!shiftable)onJikkyoStreamError(""+e);
            if(++reconnectCount>2)reconnectCount=0;
            else openSubStream();
            ctrl=null;
          }
        });
      };
    }
    if(cbDatacast){
      cbDatacast.checked=false;
      cbDatacast.onclick=()=>{
        document.querySelector(".remote-control").style.display=cbDatacast.checked?"":"none";
        if(!cbDatacast.checked){
          onDataStream=null;
          onDataStreamError=null;
          openSubStream();
          hideOnscreenButtons(false);
          bmlBrowserSetInvisible(true);
          vwrap.style.width=null;
          vwrap.style.height=null;
          checkJikkyoDisplay();
          return;
        }
        checkJikkyoDisplay();
        vwrap.style.width=vfull.clientWidth+"px";
        vwrap.style.height=vfull.clientHeight+"px";
        bmlBrowserSetVisibleSize(vcont.clientWidth,vcont.clientHeight);
        hideOnscreenButtons(true);
        bmlBrowserSetInvisible(false);
        onDataStream=(pid,dict,code,pcr)=>{
          dict[code]=bmlBrowserPlayTSSection(pid,dict[code],pcr)||dict[code];
        };
        onDataStreamError=text=>{
          document.querySelector(".remote-control-indicator").innerText=text;
        };
        openSubStream();
      };
    }
    if(cbJikkyo&&!shiftable){
      const onclickJikkyo=()=>{
        if(!cbJikkyo.onclick&&(vid.c||vid.e).currentTime==0){
          setTimeout(onclickJikkyo,500);
          return;
        }
        cbJikkyo.onclick=onclickJikkyo;
        document.querySelector('#vid-form input[name="jikkyo"]').value=cbJikkyo.checked?"1":"0";
        if(!cbJikkyo.checked){
          toggleJikkyo(false);
          openSubStream();
          setSendComment(null);
          return;
        }
        toggleJikkyo(true);
        if(postCommentQuery){
          setSendComment(commInput=>{
            if(/^@/.test(commInput.value)){
              if(commInput.value=="@sw"){
                commInput.className=commInput.className=="refuge"?"nico":"refuge";
              }
              return;
            }
            fetch("comment.lua",{
              method:"POST",
              headers:{"Content-Type":"application/x-www-form-urlencoded"},
              body:postCommentQuery+(commInput.className=="refuge"?"&refuge=1":"")+"&comm="+encodeURIComponent(commInput.value).replace(/%20/g,"+")
            }).then(response=>{
              if(!response.ok)throw new Error(response.status+" "+response.statusText);
            }).catch(e=>{
              addJikkyoMessage("Post error! ("+e+")");
            });
          });
        }
        openSubStream();
      };
      const btnConfig=document.querySelector("#jikkyo-config > button");
      btnConfig.onclick=selectID.onchange=()=>{
        jkID=+(selectID.value||"0");
        jkTM=inputTM.value?Math.floor(Date.parse(inputTM.value+"Z")/60000)*60+inputTMSec.selectedIndex-32400:0;
        openSubStream();
      };
      if(postCommentQuery){
        //Not configurable for live mode.
        inputTM.style.display="none";
        inputTMSec.style.display="none";
        btnConfig.style.display="none";
      }
      setTimeout(onclickJikkyo,postCommentQuery?5000:2000);
    }
  }
  const voffset=document.getElementById("vid-offset");
  if(voffset){
    const vselect=document.querySelector('#vid-form select[name="offset"]');
    const vthumb=document.querySelector("#vid-seek canvas");
    const vstatus=document.getElementById("vid-seek-status");
    let thumbTimer=0;
    let thumbFetching=false;
    const rangeSeekSec=n=>{
      const i=Math.floor(n);
      return Math.floor((vselect.options[vselect.options.length-101+Math.min(i+1,100)].dataset.sec||-1)*(n-i)-
                        (vselect.options[vselect.options.length-101+Math.min(i,100)].dataset.sec||-1)*(n-i-1));
    };
    const formatSec=sec=>{
      return Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s";
    };
    let mouseX=null;
    rangeSeek.ontouchend=rangeSeek.onmouseleave=()=>{
      mouseX=null;
      vseek.classList.remove("active");
      if(vthumb)vthumb.style.display="none";
      vstatus.style.display="none";
      vstatus.classList.remove("follow-thumb");
    };
    const setLeft=x=>{
      const e=vstatus.classList.contains("follow-thumb")?vthumb:vstatus;
      x-=e.offsetWidth/2;
      e.style.left=Math.floor(x)+"px";
      e.style.left=Math.floor(x-Math.min(e.getBoundingClientRect().left,0)-Math.max(e.getBoundingClientRect().right-window.innerWidth,0))+"px";
      vstatus.style.left=e.style.left;
    };
    const popup=()=>{
      //Adjust the offset between slider and mouse.
      const adjustX=(rangeSeek.clientHeight-parseFloat(getComputedStyle(rangeSeek).paddingTop)-parseFloat(getComputedStyle(rangeSeek).paddingBottom))*0.8;
      const n=Math.min(Math.max(vseek.classList.contains("active")?rangeSeek.value:(mouseX-adjustX/2)/(rangeSeek.clientWidth-adjustX)*100,0),100);
      vstatus.innerText=formatSec(currentAbsTime())+"\u2192"+
        (rangeSeekSec(n)>=0&&vid.seekWithoutTransition?formatSec(rangeSeekSec(n)):
           vselect.options[vselect.options.length-101+Math.floor(n)].textContent.match(/^(?:\d+m\d+s)?/)[0])+
        "|"+Math.floor(n)+"%";
      vstatus.style.display=null;
      setLeft(vseek.classList.contains("active")?rangeSeek.clientWidth*(rangeSeek.value/100):mouseX);
      if(vthumb&&vid.grabFirstFrame){
        clearTimeout(thumbTimer);
        thumbTimer=setTimeout(()=>{
          if((mouseX==null&&!vseek.classList.contains("active"))||thumbFetching)return;
          //Get thumbnail of seek position.
          const adjustX=(rangeSeek.clientHeight-parseFloat(getComputedStyle(rangeSeek).paddingTop)-parseFloat(getComputedStyle(rangeSeek).paddingBottom))*0.8;
          const n=Math.min(Math.max(vseek.classList.contains("active")?rangeSeek.value:(mouseX-adjustX/2)/(rangeSeek.clientWidth-adjustX)*100,0),100);
          thumbFetching=true;
          fetch("grabber.lua"+vid.initSrc.match(/\?fname=[^&]*/)[0]+(rangeSeekSec(n)>=0&&vid.seekWithoutTransition?"&ofssec="+rangeSeekSec(n):"&offset="+Math.floor(n))).then(response=>{
            if(response.ok)return response.arrayBuffer();
          }).then(arrayBuffer=>{
            if(mouseX!=null||vseek.classList.contains("active")){
              const buffer=vid.getGrabberInputBuffer(arrayBuffer.byteLength);
              buffer.set(new Uint8Array(arrayBuffer));
              const frame=vid.grabFirstFrame(arrayBuffer.byteLength);
              if(frame){
                vthumb.width=frame.width;
                vthumb.height=frame.height;
                vthumb.getContext("2d").putImageData(new ImageData(new Uint8ClampedArray(frame.buffer),frame.width,frame.height),0,0);
                vthumb.style.display=null;
                vstatus.classList.add("follow-thumb");
                setLeft(vseek.classList.contains("active")?rangeSeek.clientWidth*(rangeSeek.value/100):mouseX);
              }
            }
          }).finally(()=>{
            thumbFetching=false;
          });
        },thumbTimer?200:0);
      }
    };
    rangeSeek.oninput=()=>{
      vseek.classList.add("active");
      popup();
    };
    rangeSeek.onchange=()=>{
      vselect.options[vselect.options.length-101+Math.floor(rangeSeek.value)].selected=true;
      if(rangeSeekSec(rangeSeek.value)>=0&&vid.seekWithoutTransition){
        vid.ofssec=Math.max(rangeSeekSec(rangeSeek.value)-1,0);
        openSubStream();
        vid.seekWithoutTransition();
        mouseX=null;
        vseek.classList.remove("active");
      }else{
        document.querySelector('#vid-form button[type="submit"]').click();
      }
    };
    rangeSeek.onmousemove=e=>{
      if(mouseX!=null&&!vseek.classList.contains("active"))popup();
      mouseX=e.offsetX;
    };
    (vid.c||vid.e).ontimeupdate=()=>{
      const sec=currentAbsTime();
      voffset.innerText="|"+formatSec(sec);
      for(let i=0;;i++){
        if(i==99||(vselect.options[vselect.options.length-101+i].dataset.sec||-1)>=sec){
          const marker=document.querySelector("#vid-seek-marker option");
          if(vseek.classList.contains("active")){
            marker.value=Math.abs(i-rangeSeek.value)>5?i:null;
          }else{
            marker.value=null;
            rangeSeek.value=i;
            if(rangeSeek.style.display=="none"){
              rangeSeek.style.display=null;
              const adjustX=(rangeSeek.clientHeight-parseFloat(getComputedStyle(rangeSeek).paddingTop)-parseFloat(getComputedStyle(rangeSeek).paddingBottom))*0.8;
              for(let j=0;j<vselect.options.length-101;j++){
                const opt=vselect.options[j];
                const chapter=document.createElement("div");
                chapter.classList.add("chapter-mark");
                chapter.appendChild(document.createElement("div"));
                chapter.firstElementChild.classList.add("chapter-"+(opt.dataset.chapterIn?"in":opt.dataset.chapterOut?"out":"point"));
                const ratio=opt.dataset.sec/vselect.options[vselect.options.length-1].dataset.sec;
                chapter.style.left=Math.floor(1000*ratio)/10+"%";
                chapter.style.transform="translateX(-50%) translateX("+Math.floor(adjustX*(0.5-ratio))+"px)";
                chapter.onmouseenter=()=>{
                  vstatus.style.display=null;
                  vstatus.innerText=opt.textContent;
                  setLeft(rangeSeek.clientWidth*ratio);
                };
                chapter.onmouseleave=()=>{
                  vstatus.style.display="none";
                };
                chapter.onclick=()=>{
                  opt.selected=true;
                  if(vid.seekWithoutTransition){
                    vid.ofssec=Math.max(opt.dataset.sec-1,0);
                    openSubStream();
                    vid.seekWithoutTransition();
                  }else{
                    document.querySelector('#vid-form button[type="submit"]').click();
                  }
                };
                document.getElementById("vid-seek-popup").appendChild(chapter);
              }
            }
          }
          break;
        }
      }
    };
    voffset.innerText="|"+formatSec(vid.ofssec);
  }
  const vfast=document.querySelector('#vid-form select[name="fast"]');
  if(vfast){
    vfast.onchange=()=>{
      if(vfast.selectedIndex>=0&&vid.seekWithoutTransition){
        vid.ofssec=currentAbsTime();
        vid.fastParam="&fast="+vfast.options[vfast.selectedIndex].value;
        vid.fast=1*vfast.options[vfast.selectedIndex].textContent.substring(1);
        openSubStream();
        vid.seekWithoutTransition();
      }
    };
  }
  if(!vid.c){
    let swtCount=0;
    vid.seekWithoutTransition=()=>{
      //"count" is to ensure that the src attribute is reloaded.
      vid.e.src=(vid.fastParam?vid.initSrc.replace(/&fast=[^&]*/,"")+vid.fastParam:vid.initSrc).replace("&load=","&reload=")+"&ofssec="+vid.ofssec+"&count="+(++swtCount);
    };
    try{
      const canvas=document.createElement("canvas");
      const m=(vid.e.dataset.poster||"").match(/^(\d+)x(\d+)(,.*|)$/);
      canvas.width=m?parseInt(m[1],10)||1280:1280;
      canvas.height=m?parseInt(m[2],10)||720:720;
      const ctx=canvas.getContext("2d");
      ctx.fillStyle=getComputedStyle(vid.e).getPropertyValue("--poster-bg-color")||"gray";
      ctx.fillRect(0,0,canvas.width,canvas.height);
      ctx.fillStyle=getComputedStyle(vid.e).getPropertyValue("--poster-color")||"white";
      ctx.textAlign="center";
      ctx.font=canvas.height/10+"px sans-serif";
      ctx.fillText(m&&m[3]?m[3].substring(1):"Loading...",canvas.width/2,canvas.height/2);
      vid.e.poster=canvas.toDataURL();
    }catch(e){
      console.warn("poster:",e);
    }
  }
  seekVideo=(sec)=>{
    if(vid.seekWithoutTransition){
      vid.ofssec=Math.floor(sec);
      openSubStream();
      vid.seekWithoutTransition();
    }
  };
};

const runHlsScript=()=>{
  let cap=null;
  const cbCaption=document.getElementById("cb-caption");
  const onclickCaption=()=>{
    if(cbCaption.checked){
      if(!cap){
        let aribb24Option=null;
        try{
          aribb24Option=JSON.parse(decodeURIComponent(cbCaption.dataset.aribb24OptionJson));
        }catch(e){
          console.warn("aribb24OptionJson:",e);
        }
        aribb24Option.enableAutoInBandMetadataTextTrackDetection=!vid.e.dataset.alwaysUseHls||!Hls.isSupported();
        try{
          cap=cbCaption.dataset.aribb24UseSvg?new aribb24js.SVGRenderer(aribb24Option):new aribb24js.CanvasRenderer(aribb24Option);
          cap.attachMedia(vid.e);
        }catch(e){
          cap=null;
          console.warn("aribb24js:",e);
        }
      }
      if(cap)cap.show();
    }else if(cap){
      cap.hide();
    }
    document.querySelector('#vid-form input[name="caption"]').value=cbCaption.checked?"1":"0";
  };
  if(vid.e.dataset.alwaysUseHls){
    vid.seekWithoutTransition=null;
    onclickCaption();
    cbCaption.onclick=onclickCaption;
    document.getElementById("label-caption").style.display="inline";
    const cbLive=document.getElementById("cb-live");
    if(cbLive)cbLive.checked=true;
    waitForHlsStart(vid.initSrc+
      //Excludes Firefox for Android, because playback of non-keyframe fragmented MP4 is jerky.
      "&hls="+vid.e.dataset.hls+(!vid.e.dataset.hlsMp4||/Android.+Firefox/i.test(navigator.userAgent)?"":"&hls4="+vid.e.dataset.hlsMp4),
      "ctok="+vid.e.dataset.ctok+"&open=1",200,500).then(src=>{
      if(Hls.isSupported()){
        const hls=new Hls({workerPath:"hls.worker.js"});
        hls.loadSource(src);
        hls.attachMedia(vid.e);
        hls.on(Hls.Events.MANIFEST_PARSED,()=>{vid.e.play();});
        hls.on(Hls.Events.FRAG_PARSING_METADATA,(event,data)=>{
          if(cap){
            for(const sample of data.samples){cap.pushID3v2Data(sample.pts,sample.data);}
          }
        });
        const vbitrate=document.getElementById("vid-bitrate");
        vbitrate.innerText="|?Mbps";
        let t=-1;
        let total=0;
        hls.on(Hls.Events.FRAG_BUFFERED,(event,data)=>{
          if(data.stats){
            const now=data.stats.buffering.end;
            if(t<0)t=now;
            else total+=data.stats.total;
            if(now-t>7000){
              vbitrate.innerText="|"+(total*1000/((now-t)*1024*128)).toFixed(1)+"Mbps";
              t=now;
              total=0;
            }
          }
        });
        let swtCount=0;
        const swt=()=>{
          vid.seekWithoutTransition=null;
          hls.detachMedia();
          waitForHlsStart((vid.fastParam?vid.initSrc.replace(/&fast=[^&]*/,"")+vid.fastParam:vid.initSrc).replace("&load=","&reload=")+"&ofssec="+vid.ofssec+
            //Excludes Firefox for Android, because playback of non-keyframe fragmented MP4 is jerky.
            "&hls="+(++swtCount)+"_"+vid.e.dataset.hls+(!vid.e.dataset.hlsMp4||/Android.+Firefox/i.test(navigator.userAgent)?"":"&hls4="+vid.e.dataset.hlsMp4),
            "ctok="+vid.e.dataset.ctok+"&open=1",200,500).then(src=>{
            hls.loadSource(src);
            hls.attachMedia(vid.e);
            vid.seekWithoutTransition=swt;
          }).catch(()=>{
            vid.e.poster=null;
          });
        };
        vid.seekWithoutTransition=swt;
      }else if(vid.e.canPlayType("application/vnd.apple.mpegurl")){
        vid.e.src=src;
      }
    }).catch(()=>{
      vid.e.poster=null;
    });
  }else{
    //Excludes Android even though canPlayType here may not return an empty string, because the quality of the native implementation is inconsistent.
    if(!/Android/i.test(navigator.userAgent)&&vid.e.canPlayType("application/vnd.apple.mpegurl")){
      vid.seekWithoutTransition=null;
      onclickCaption();
      cbCaption.onclick=onclickCaption;
      document.getElementById("label-caption").style.display="inline";
      const cbLive=document.getElementById("cb-live");
      if(cbLive)cbLive.checked=true;
      waitForHlsStart(vid.initSrc+"&hls="+vid.e.dataset.hls+(!vid.e.dataset.hlsMp4?"":"&hls4="+vid.e.dataset.hlsMp4),"ctok="+vid.e.dataset.ctok+"&open=1",200,500).then(src=>{
        vid.e.src=src;
      }).catch(()=>{
        vid.e.poster=null;
      });
    }else{
      vid.e.src=vid.initSrc;
    }
  }
};

const runTsliveScript=()=>{
  const vbitrate=document.getElementById("vid-bitrate");
  let bitrateStart=null;
  let bitrateTotal=0;
  let lastWidth=vid.e.width;
  let lastHeight=vid.e.height;
  let wakeLock=null;
  let abortState="";
  const readNext=(mod,reader,ret)=>{
    if(ret&&ret.value&&!abortState){
      const inputLen=Math.min(ret.value.length,1e6);
      const buffer=mod.getNextInputBuffer(inputLen);
      if(!buffer){
        setTimeout(()=>{readNext(mod,reader,ret);},1000);
        return;
      }
      buffer.set(new Uint8Array(ret.value.buffer,ret.value.byteOffset,inputLen));
      mod.commitInputData(inputLen);
      if(inputLen<ret.value.length){
        //Input the rest.
        setTimeout(()=>{readNext(mod,reader,{value:new Uint8Array(ret.value.buffer,ret.value.byteOffset+inputLen,ret.value.length-inputLen)});},0);
        return;
      }
    }
    reader.read().then(r=>{
      if(r.done){
        if(wakeLock)wakeLock.release();
        vid.seekWithoutTransition=null;
        vid.e.onclick=null;
        if(abortState=="seeking"){
          startRead(mod);
        }else if(abortState=="paused"){
          vid.seekWithoutTransition=()=>startRead(mod);
          vid.e.onclick=()=>seekVideo(vid.ofssec);
        }
      }else{
        const now=Date.now();
        if(!bitrateStart)bitrateStart=now;
        bitrateTotal+=r.value.length;
        if(now-bitrateStart>7000){
          vbitrate.innerText="|"+(bitrateTotal*1000/((now-bitrateStart)*1024*128)).toFixed(1)+"Mbps";
          bitrateStart=now;
          bitrateTotal=0;
        }
        readNext(mod,reader,r);
      }
    }).catch(e=>{
      if(wakeLock)wakeLock.release();
      vid.seekWithoutTransition=null;
      vid.e.onclick=null;
      if(abortState=="seeking"){
        startRead(mod);
      }else if(abortState=="paused"){
        vid.seekWithoutTransition=()=>startRead(mod);
        vid.e.onclick=()=>seekVideo(vid.ofssec);
      }
      throw e;
    });
    if(lastWidth!=vid.e.width||lastHeight!=vid.e.height){
      lastWidth=vid.e.width;
      lastHeight=vid.e.height;
      adjustVideoMaxWidth();
    }
  };
  let cap=null;
  const cbCaption=document.getElementById("cb-caption");
  const onclickCaption=()=>{
    if(cbCaption.checked){
      if(!cap){
        let aribb24Option=null;
        try{
          aribb24Option=JSON.parse(decodeURIComponent(cbCaption.dataset.aribb24OptionJson));
        }catch(e){
          console.warn("aribb24OptionJson:",e);
        }
        try{
          cap=cbCaption.dataset.aribb24UseSvg?new aribb24js.SVGRenderer(aribb24Option):new aribb24js.CanvasRenderer(aribb24Option);
          cap.attachMedia(null,vcont);
        }catch(e){
          cap=null;
          console.warn("aribb24js:",e);
        }
      }
      if(cap)cap.show();
    }else if(cap){
      cap.hide();
    }
    document.querySelector('#vid-form input[name="caption"]').value=cbCaption.checked?"1":"0";
  };
  onclickCaption();
  cbCaption.onclick=onclickCaption;
  document.getElementById("label-caption").style.display="inline";

  const startRead=mod=>{
    vid.seekWithoutTransition=null;
    vid.e.onclick=null;
    if(abortState=="paused"){
      mod.resume();
    }
    let deinterlace=vid.e.dataset.deinterlace;
    if(deinterlace&&mod.setDeinterlace){
      if(vid.fast>(vid.e.dataset.maxRateForDoubling||1)){
        deinterlace=deinterlace.replace("=1,","=0,").replace(/=1$/,"");
      }
      mod.setDeinterlace(deinterlace);
    }
    mod.setPlaybackRate(vid.fast);
    mod.reset();
    const ctrl=new AbortController();
    //"throttle" is to avoid excessive prefetching in some browsers.
    fetch((vid.fastParam?vid.initSrc.replace(/&fast=[^&]*/,"")+vid.fastParam:vid.initSrc)+
          (abortState?"&ofssec="+vid.ofssec:"")+"&throttle=1",{signal:ctrl.signal}).then(response=>{
      if(!response.ok)return;
      //Reset caption
      if(cap)cap.attachMedia(null,vcont);
      vid.currentTime=0;
      vid.seekWithoutTransition=()=>{
        vid.currentTime=0;
        vid.seekWithoutTransition=null;
        vid.e.onclick=null;
        abortState="seeking";
        ctrl.abort();
      };
      if(vid.e.dataset.pausable&&mod.pause){
        vid.e.onclick=()=>{
          vid.ofssec+=Math.floor(vid.currentTime);
          vid.currentTime=0;
          vid.seekWithoutTransition=null;
          vid.e.onclick=null;
          mod.pause();
          abortState="paused";
          ctrl.abort();
        };
      }
      readNext(mod,response.body.getReader(),null);
      //Prevent screen sleep
      navigator.wakeLock.request("screen").then(lock=>{wakeLock=lock;});
    });
    abortState="";
  };
  const notify=s=>{
    const ctx=vid.e.getContext("2d");
    ctx.fillStyle="black";
    ctx.fillText(s,10,30);
    ctx.fillStyle="white";
    ctx.fillText(s,10,50);
  };
  if(!window.createWasmModule){
    notify("Error! Probably ts-live.js not found.");
    return;
  }
  if(!navigator.gpu){
    notify("Error! WebGPU not available.");
    return;
  }
  const rangeVolume=document.getElementById("vid-volume");
  rangeVolume.style.display=null;
  navigator.gpu.requestAdapter().then(adapter=>{
    adapter.requestDevice().then(device=>{
      createWasmModule({preinitializedWebGPUDevice:device}).then(mod=>{
        //Functions for drawing thumbnails
        vid.getGrabberInputBuffer=mod.getGrabberInputBuffer;
        vid.grabFirstFrame=mod.grabFirstFrame;
        let statsTime=0;
        mod.setCaptionCallback((pts,ts,data)=>{
          if(cap)cap.pushRawData(statsTime+ts,data.slice());
        });
        mod.setAudioGain(vid.muted?0:vid.volume);
        rangeVolume.value=Math.floor((vid.muted?0:vid.volume)*100);
        vid.unmute=()=>{
          vid.muted=false;
          rangeVolume.value=Math.floor(vid.volume*100);
          mod.setAudioGain(vid.volume);
        };
        rangeVolume.oninput=rangeVolume.onchange=()=>{
          const btnUnmute=document.getElementById("vid-unmute");
          btnUnmute.style.display="none";
          vid.muted=false;
          vid.volume=rangeVolume.value/100;
          mod.setAudioGain(vid.volume);
        };
        const cbAudio2=document.querySelector('#vid-form input[name="audio2"]');
        //2nd audio channel
        mod.setDualMonoMode(cbAudio2.checked?1:0);
        cbAudio2.onclick=()=>{mod.setDualMonoMode(cbAudio2.checked?1:0);};
        const cbCinema=document.querySelector('#vid-form input[name="cinema"]');
        let autoCinema=!!vid.e.dataset.initialAutoCinema;
        if(mod.setDetelecineMode){
          //0=never,1=force,2=auto
          mod.setDetelecineMode(autoCinema?2:cbCinema.checked?1:0);
          cbCinema.onclick=()=>{
            autoCinema=false;
            mod.setDetelecineMode(cbCinema.checked?1:0);
          };
        }else{
          autoCinema=false;
        }
        let statCount=0;
        let renderedCount=0;
        let prevNow=performance.now();
        mod.setStatsCallback(stats=>{
          if(statsTime!=stats[stats.length-1].time){
            vid.currentTime+=stats[stats.length-1].time-statsTime;
            statsTime=stats[stats.length-1].time;
            if(vid.ontimeupdate)vid.ontimeupdate();
            if(cap)cap.onTimeupdate(statsTime);
          }
          if(autoCinema){
            const f=stats[stats.length-1].TelecineFlag;
            if(cbCinema.checked!=f)cbCinema.checked=f;
          }
          statCount+=stats.length-1;
          for(const stat of stats){
            renderedCount+=stat.RenderedFlag?1:0;
          }
          const now=performance.now();
          if(now-prevNow>10000){
            console.log("stat "+(renderedCount*1000/(now-prevNow))+" fps "+(statCount*1000/(now-prevNow))+" rAF calls/s");
            statCount=0;
            renderedCount=0;
            prevNow=now;
          }
        });
        setTimeout(()=>{
          vbitrate.innerText="|?Mbps";
          startRead(mod);
        },500);
      });
    });
  }).catch(e=>{
    notify(e.message);
    throw e;
  });
};
