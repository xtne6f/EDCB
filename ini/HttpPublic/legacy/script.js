function readPsiData(data,proc,startSec,ctx){
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
    var pos=ctx.pos+ctx.trailerSize;
    var timeListLen=data.getUint16(pos+10,true);
    var dictionaryLen=data.getUint16(pos+12,true);
    var dictionaryWindowLen=data.getUint16(pos+14,true);
    var dictionaryDataSize=data.getUint32(pos+16,true);
    var dictionaryBuffSize=data.getUint32(pos+20,true);
    var codeListLen=data.getUint32(pos+24,true);
    if(data.getUint32(pos)!=0x50737363||
       data.getUint32(pos+4)!=0x0d0a9a0a||
       dictionaryWindowLen<dictionaryLen||
       dictionaryBuffSize<dictionaryDataSize||
       dictionaryWindowLen>65536-4096){
      return null;
    }
    var chunkSize=32+timeListLen*4+dictionaryLen*2+Math.ceil(dictionaryDataSize/2)*2+codeListLen*2;
    if(data.byteLength-pos<chunkSize)break;
    var timeListPos=pos+32;
    pos+=32+timeListLen*4;
    if(ctx.timeListCount<0){
      var pids=[];
      var dict=[];
      var sectionListPos=0;
      for(var i=0;i<dictionaryLen;i++,pos+=2){
        var codeOrSize=data.getUint16(pos,true)-4096;
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
      for(var i=0;i<dictionaryLen;i++){
        if(pids[i]>=0)continue;
        dict[i]=new Uint8Array(data.buffer.slice(sectionListPos,sectionListPos+pids[i]+4097));
        sectionListPos+=pids[i]+4097;
        pids[i]=data.getUint16(pos,true)&0x1fff;
        pos+=2;
      }
      for(var i=dictionaryLen,j=0;i<dictionaryWindowLen;j++){
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
      var initTime=ctx.initTime;
      var currTime=ctx.currTime;
      var absTime=data.getUint32(timeListPos,true);
      if(absTime==0xffffffff){
        currTime=-1;
      }else if(absTime>=0x80000000){
        currTime=absTime&0x3fffffff;
        if(initTime<0)initTime=currTime;
      }else{
        var n=data.getUint16(timeListPos+2,true)+1;
        if(currTime>=0){
          currTime+=data.getUint16(timeListPos,true);
          var sec=((currTime+0x40000000-initTime)&0x3fffffff)/11250;
          if(sec>=(startSec||0)){
            for(;ctx.codeCount<n;ctx.codeCount++,pos+=2,ctx.codeListPos+=2){
              var code=data.getUint16(pos,true)-4096;
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
  var ret=data.buffer.slice(ctx.pos);
  ctx.pos=0;
  return ret;
}

function progressPsiDataChatMixedStream(readCount,response,onData,onChat,ctx){
  ctx=ctx||{};
  if(!ctx.ctx){
    ctx.ctx={};
    ctx.atobRemain="";
    ctx.psiData=new Uint8Array(0);
  }
  while(readCount<response.length){
    var i=response.indexOf("<",readCount);
    if(i==readCount){
      i=response.indexOf("\n",readCount);
      if(i<0)break;
      if(onChat)onChat(response.substring(readCount,i));
      readCount=i+1;
    }else{
      i=i<0?response.length:i;
      var n=Math.floor((i-readCount+ctx.atobRemain.length)/4)*4;
      if(n){
        var addData=atob(ctx.atobRemain+response.substring(readCount,readCount+n-ctx.atobRemain.length));
        ctx.atobRemain=response.substring(readCount+n-ctx.atobRemain.length,i);
        var concatData=new Uint8Array(ctx.psiData.length+addData.length);
        for(var j=0;j<ctx.psiData.length;j++)concatData[j]=ctx.psiData[j];
        for(var j=0;j<addData.length;j++)concatData[ctx.psiData.length+j]=addData.charCodeAt(j);
        ctx.psiData=readPsiData(concatData.buffer,function(sec,dict,code,pid){
          if(onData)onData(pid,dict,code,Math.floor(sec*90000));
          return true;
        },0,ctx.ctx);
        if(ctx.psiData)ctx.psiData=new Uint8Array(ctx.psiData);
      }else{
        atobRemain+=response.substring(readCount,i);
      }
      readCount=i;
    }
  }
  return readCount;
}

function decodeB24CaptionFromCueText(text,work){
  work=work||[];
  text=text.replace(/\r?\n/g,'');
  var re=/<v b24caption[0-8]>(.*?)<\/v>/g;
  var src,ret=null;
  while((src=re.exec(text))!==null){
    src=src[1].replace(/<.*?>/g,'').replace(/&(?:amp|lt|gt|quot|apos);/g,function(m){
      return m=='&amp;'?'&':m=='&lt;'?'<':m=='&gt;'?'>':m=='&quot;'?'"':'\'';
    });
    var brace=[],wl=0,hi=0;
    for(var i=0;i<src.length;){
      if(src[i]=='%'){
        if((++i)+2>src.length)return null;
        var c=src[i++];
        var d=src[i++];
        if(c=='^'){
          work[wl++]=0xc2;
          work[wl++]=d.charCodeAt(0)+64;
        }else if(c=='='){
          if(d=='{'){
            work[wl++]=0;
            work[wl++]=0;
            work[wl++]=0;
            brace.push(wl);
          }else if(d=='}'&&brace.length>0){
            var pos=brace.pop();
            work[pos-3]=wl-pos>>16&255;
            work[pos-2]=wl-pos>>8&255;
            work[pos-1]=wl-pos&255;
          }else return null;
        }else if(c=='+'){
          if(d=='{'){
            var pos=src.indexOf('%+}',i);
            if(pos<0)return null;
            try{
              var buf=atob(src.substring(i,pos));
              for(var j=0;j<buf.length;j++)work[wl++]=buf.charCodeAt(j);
            }catch(e){return null;}
            i=pos+3;
          }else return null;
        }else{
          var x=c.charCodeAt(0);
          var y=d.charCodeAt(0);
          work[wl++]=(x>=97?x-87:x>=65?x-55:x-48)<<4|(y>=97?y-87:y>=65?y-55:y-48);
        }
      }else{
        var x=src.charCodeAt(i++);
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
      var r=new Uint8Array(wl+7);
      r[0]=0x80;
      r[1]=0xff;
      r[2]=0xf0;
      r[3]=work[0];
      r[4]=work[1];
      r[5]=work[2];
      r[6]=wl-3>>8&255;
      r[7]=wl-3&255;
      for(var i=3;i<wl;i++)r[i+5]=work[i];
      ret=ret||[];
      ret.push(r);
    }
  }
  return ret;
}

function waitForHlsStart(src,interval,delay,onerror,onstart){
  (function poll(){
    var xhr=new XMLHttpRequest();
    xhr.open("GET",src);
    xhr.onload=function(){
      if(xhr.status==200&&xhr.response){
        if(xhr.response.indexOf('#EXT-X-MEDIA-SEQUENCE:')<0)setTimeout(poll,interval);
        else setTimeout(function(){onstart(src);},delay);
      }else{
        onerror();
      }
    }
    xhr.send();
  })();
}

function unescapeHtml(s){
  return s.replace(/&(?:amp|lt|gt|quot|apos|#10|#13);/g,function(m){
    return m[1]=="l"?"<":m[1]=="g"?">":m[1]=="q"?'"':m[3]=="p"?"&":m[3]=="o"?"'":m[3]=="0"?"\n":"\r";
  });
}

var chatTagColors={
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

var getChatTagColorRe=new RegExp("(?:^| )(#[0-9A-Fa-f]{6}|"+Object.keys(chatTagColors).join("|")+")(?: |$)");

function parseChatTag(tag){
  var m=tag.match(/^<chat(?= )(.*)>(.*?)<\/chat>$/);
  if(m){
    var a=m[1];
    var r={text:unescapeHtml(m[2])};
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
        m=a.match(/ user_id="([0-9A-Za-z_-]*)"/);
        r.user=m?m[1]:"";
        return r;
      }
    }
  }
  return null;
}

function readJikkyoLog(text,proc,startSec,ctx){
  ctx=ctx||{};
  if(ctx.pos===undefined){
    ctx.pos=0;
    ctx.currSec=-1;
  }
  for(;;){
    var i=text.indexOf("\n",ctx.pos);
    if(i<0)break;
    var tag=text.substring(ctx.pos,i);
    var sec=ctx.currSec;
    if(/^<!-- J=/.test(tag))sec++;
    if(sec>=(startSec||0)&&!proc(sec,tag))break;
    ctx.pos=i+1;
    ctx.currSec=sec;
  }
}

function runOnscreenButtonsScript(){
  var btn=document.createElement("button");
  btn.type="button";
  btn.innerText="full";
  btn.onclick=function(){(vfull.requestFullscreen||vfull.webkitRequestFullscreen||vfull.webkitRequestFullScreen).call(vfull);};
  var bfull=document.createElement("div");
  bfull.className="full-control";
  bfull.appendChild(btn);
  btn=document.createElement("button");
  btn.type="button";
  btn.innerText="exit";
  btn.onclick=function(){(document.exitFullscreen||document.webkitExitFullscreen||document.webkitCancelFullScreen).call(document);};
  var bexit=document.createElement("div");
  bexit.className="exit-control";
  bexit.appendChild(btn);
  var diffs=[0,0,0,0,0];
  var duration=-1;
  var lastseek=0;
  function checkDuration(){
    var seekable=vid.duration;
    if(seekable==Infinity)seekable=vid.seekable.length>0?vid.seekable.end(vid.seekable.length-1):0;
    if(!(seekable>0))return;
    if(duration<0)duration=seekable;
    if(seekable-duration<0.5)return;
    diffs.shift();
    diffs.push(seekable-duration);
    duration=seekable;
    var interval=Math.max(diffs[0],diffs[1],diffs[2],diffs[3],diffs[4])+1;
    if(vid.currentTime<duration-interval*2-3&&Date.now()-lastseek>10000){
      var cbLive=document.getElementById("cb-live");
      if(cbLive&&cbLive.checked){
        vid.currentTime=duration-interval;
        lastseek=Date.now();
      }
    }
  }
  vid.ondurationchange=checkDuration;
  setInterval(checkDuration,500);
  btn=document.createElement("button");
  btn.type="button";
  btn.innerText="\u2192";
  btn.onclick=function(){vid.currentTime=duration-Math.max(diffs[0],diffs[1],diffs[2],diffs[3],diffs[4])-1;};
  var blive=document.createElement("div");
  blive.className="live-control";
  blive.appendChild(btn);
  var commInput=document.createElement("input");
  commInput.type="text";
  var commSend=null;
  commInput.onkeydown=function(e){
    if(!e.isComposing&&e.keyCode!=229&&e.key=="Enter"){
      if(commSend&&commInput.value)commSend(commInput.value);
      commInput.value="";
    }
  };
  var btn=document.createElement("button");
  btn.type="button";
  btn.innerText="\u226b";
  btn.onclick=function(){
    if(commSend&&commInput.value)commSend(commInput.value);
    commInput.value="";
  };
  var bcomm=document.createElement("div");
  bcomm.className="comment-control";
  bcomm.style.display="none";
  bcomm.appendChild(commInput);
  bcomm.appendChild(btn);
  setSendComment=function(f){
    bcomm.style.display=f?null:"none";
    commSend=f;
  };
  var removed=true;
  hideOnscreenButtons=function(hide){
    if(!removed&&hide){
      vcont.removeChild(bfull);
      vcont.removeChild(bexit);
      vcont.removeChild(blive);
      vcont.removeChild(bcomm);
      removed=true;
    }else if(removed&&!hide){
      vcont.appendChild(bfull);
      vcont.appendChild(bexit);
      vcont.appendChild(blive);
      vcont.appendChild(bcomm);
      removed=false;
    }
  };
  hideOnscreenButtons(false);
}

function runJikkyoScript(commentHeight,commentDuration,replaceTag){
  var danmaku=null;
  var comm=null;
  var cbJikkyoOnscr=document.getElementById("cb-jikkyo-onscr");
  function onclickJikkyoOnscr(){
    if(comm&&comm.style.display!="none"){
      if(cbJikkyoOnscr.checked)danmaku.show();
      else danmaku.hide();
    }
  }
  cbJikkyoOnscr.onclick=onclickJikkyoOnscr;
  checkJikkyoDisplay=function(){
    if(comm){
      var cbDatacast=document.getElementById("cb-datacast");
      var cbJikkyo=document.getElementById("cb-jikkyo");
      if((cbDatacast&&cbDatacast.checked)||!cbJikkyo.checked){
        if(comm.style.display!="none"){
          danmaku.hide();
          comm.style.display="none";
        }
      }else if(comm.style.display=="none"){
        vfull.appendChild(comm);
        comm.style.display=null;
        onclickJikkyoOnscr();
      }
    }
  };
  toggleJikkyo=function(enabled){
    if(!enabled){
      onJikkyoStream=null;
      onJikkyoStreamError=null;
      checkJikkyoDisplay();
      return;
    }
    if(!comm){
      comm=document.createElement("div");
      comm.className="jikkyo-comments";
      vfull.appendChild(comm);
    }
    if(!danmaku){
      danmaku=new Danmaku({
        container:vcont,
        opacity:1,
        callback:function(){},
        error:function(msg){},
        apiBackend:{read:function(opt){opt.success([]);}},
        height:commentHeight,
        duration:commentDuration,
        paddingTop:10,
        paddingBottom:10,
        unlimited:false,
        api:{id:"noid",address:"noad",token:"noto",user:"nous",speedRate:1}
      });
    }
    checkJikkyoDisplay();
    function addMessage(text){
      var b=document.createElement("strong");
      b.innerText=text;
      var div=document.createElement("div");
      div.appendChild(b);
      comm.appendChild(div);
    }
    var commHide=true;
    setInterval(function(){
      if(getComputedStyle(comm).display=="none"){
        commHide=true;
      }else{
        var scroll=Math.abs(comm.scrollTop+comm.clientHeight-comm.scrollHeight)<comm.clientHeight/4;
        comm.style.height=vid.clientHeight+"px";
        if(commHide||scroll)comm.scrollTop=comm.scrollHeight;
        commHide=false;
      }
    },1000);
    var fragment=null;
    var scatter=[];
    var scatterInterval=200;
    var closed=false;
    onJikkyoStream=function(tag){
      if(/^<chat /.test(tag)){
        var c=parseChatTag(replaceTag(tag));
        if(c){
          if(c.yourpost)c.border="2px solid #c00";
          scatter.push(c);
          var b=document.createElement(c.yourpost?"strong":"b");
          b.innerText=String(100+(Math.floor(c.date/3600)+9)%24).substring(1)+":"+
                      String(100+Math.floor(c.date/60)%60).substring(1)+":"+
                      String(100+c.date%60).substring(1)+" ("+c.user.substring(0,3)+") ";
          var span=document.createElement("span");
          span.innerText=c.text;
          if(c.color!=0xffffff){
            span.style.backgroundColor=c.colorcode;
            span.className=(c.color>>16)*3+(c.color>>8)%256*6+c.color%256<255?"dark":"light";
          }
          var div=document.createElement("div");
          if(closed){
            div.className="closed";
            closed=false;
          }
          div.appendChild(b);
          div.appendChild(span);
          if(!fragment)fragment=document.createDocumentFragment();
          fragment.appendChild(div);
        }
        return;
      }else if(/^<chat_result /.test(tag)){
        var m=tag.match(/^[^>]*? status="(\d+)"/);
        if(m&&m[1]!="0")addMessage("Error! (chat_result="+m[1]+")");
        return;
      }else if(/^<!-- M=/.test(tag)){
        if(tag.substring(7,22)=="Closed logfile.")closed=true;
        else if(tag.substring(7,31)!="Started reading logfile:")addMessage(tag.substring(7,tag.length-4));
        return;
      }else if(!/^<!-- J=/.test(tag)){
        return;
      }
      if(tag.indexOf(";T=")<0)scatterInterval=90;
      else scatterInterval=Math.min(Math.max(scatterInterval+(scatter.length>0?-10:10),100),200);
      setTimeout(function(){
        var scroll=Math.abs(comm.scrollTop+comm.clientHeight-comm.scrollHeight)<comm.clientHeight/4;
        if(fragment){
          comm.appendChild(fragment);
          fragment=null;
        }
        if(scatterInterval<100){
          danmaku.draw(scatter);
          scatter.splice(0);
        }
        var n=Math.ceil(scatter.length/5);
        if(n>0){
          for(var i=0;i<5;i++){
            setTimeout(function(){
              if(scatter.length>0){
                danmaku.draw(scatter.slice(0,n));
                scatter.splice(0,n);
              }
            },scatterInterval*i);
          }
        }
        if(commHide||scroll){
          while(comm.childElementCount>1000){
            comm.removeChild(comm.firstElementChild);
          }
        }
        if(scroll)comm.scrollTop=comm.scrollHeight;
      },0);
    };
    onJikkyoStreamError=function(status,readCount){
      addMessage("Error! ("+status+"|"+readCount+"Bytes)");
    };
  };
}

function runVideoScript(aribb24UseSvg,aribb24Option,useDatacast,useJikkyoLog){
  var cap=null;
  var cbCaption=document.getElementById("cb-caption");
  cbCaption.onclick=function(){
    if(cap){if(cbCaption.checked){cap.show();}else{cap.hide();}}
  };
  var vidMeta=document.getElementById("vid-meta");
  vidMeta.oncuechange=function(){
    vidMeta.oncuechange=null;
    var work=[];
    var dataList=[];
    var cues=vidMeta.track.cues;
    for(var i=0;i<cues.length;i++){
      var ret=decodeB24CaptionFromCueText(cues[i].text,work);
      if(!ret){return;}
      for(var j=0;j<ret.length;j++){dataList.push({pts:cues[i].startTime,pes:ret[j]});}
    }
    cap=aribb24UseSvg?new aribb24js.SVGRenderer(aribb24Option):new aribb24js.CanvasRenderer(aribb24Option);
    cap.attachMedia(vid);
    document.getElementById("label-caption").style.display="inline";
    if(!cbCaption.checked){cap.hide();}
    dataList.reverse();
    (function pushCap(){
      for(var i=0;i<100;i++){
        var data=dataList.pop();
        if(!data){return;}
        cap.pushRawData(data.pts,data.pes);
      }
      setTimeout(pushCap,0);
    })();
  };
  if(useDatacast){
    (function(){
      var psiData=null;
      var readTimer=null;
      var videoLastSec=0;
      function startRead(){
        clearTimeout(readTimer);
        var startSec=vid.currentTime;
        videoLastSec=startSec;
        var ctx={};
        function read(){
          var videoSec=vid.currentTime;
          if(videoSec<videoLastSec||videoLastSec+10<videoSec){
            startRead();
            return;
          }
          videoLastSec=videoSec;
          if(psiData&&readPsiData(psiData,function(sec,dict,code,pid){
              dict[code]=bmlBrowserPlayTSSection(pid,dict[code],Math.floor(sec*90000))||dict[code];
              return sec<videoSec;
            },startSec,ctx)!==false){
            startRead();
            return;
          }
          readTimer=setTimeout(read,500);
        }
        readTimer=setTimeout(read,500);
      }
      var xhr=null;
      var cbDatacast=document.getElementById("cb-datacast");
      cbDatacast.checked=false;
      cbDatacast.onclick=function(){
        document.querySelector(".remote-control").style.display=cbDatacast.checked?"":"none";
        if(!cbDatacast.checked){
          clearTimeout(readTimer);
          readTimer=null;
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
        if(xhr)return;
        xhr=new XMLHttpRequest();
        xhr.open("GET",vid.getAttribute("src").replace(/\.[0-9A-Za-z]+$/,"")+".psc");
        xhr.responseType="arraybuffer";
        xhr.overrideMimeType("application/octet-stream");
        xhr.onloadend=function(){
          if(!psiData){
            document.querySelector(".remote-control-indicator").innerText="Error! ("+xhr.status+")";
          }
        };
        xhr.onload=function(){
          if(xhr.status!=200||!xhr.response)return;
          psiData=xhr.response;
        };
        xhr.send();
      };
    })();
  }
  if(useJikkyoLog){
    (function(){
      var logText=null;
      var readTimer=null;
      var videoLastSec=0;
      function startRead(){
        clearTimeout(readTimer);
        var startSec=vid.currentTime;
        videoLastSec=startSec;
        var ctx={};
        function read(){
          var videoSec=vid.currentTime;
          if(videoSec<videoLastSec||videoLastSec+10<videoSec){
            startRead();
            return;
          }
          videoLastSec=videoSec;
          if(logText){
            readJikkyoLog(logText,function(sec,tag){
              if(onJikkyoStream)onJikkyoStream(tag);
              return sec<videoSec;
            },startSec,ctx);
          }
          readTimer=setTimeout(read,200);
        }
        readTimer=setTimeout(read,200);
      }
      var xhr=null;
      var cbJikkyo=document.getElementById("cb-jikkyo");
      function onclickJikkyo(){
        cbJikkyo.onclick=onclickJikkyo;
        if(!cbJikkyo.checked){
          toggleJikkyo(false);
          clearTimeout(readTimer);
          readTimer=null;
          return;
        }
        toggleJikkyo(true);
        startRead();
        if(xhr)return;
        xhr=new XMLHttpRequest();
        xhr.open("GET","jklog.lua?fname="+vid.getAttribute("src").replace(/^(?:\.\.\/)+/,""));
        xhr.onloadend=function(){
          if(!logText){
            if(onJikkyoStreamError)onJikkyoStreamError(xhr.status,0);
          }
        };
        xhr.onload=function(){
          if(xhr.status!=200||!xhr.response)return;
          logText=xhr.response;
        };
        xhr.send();
      }
      setTimeout(onclickJikkyo,500);
    })();
  }
}

function runTranscodeScript(useDatacast,useLiveJikkyo,useJikkyoLog,ofssec,fast,postCommentQuery){
  if(useDatacast||useLiveJikkyo||useJikkyoLog){
    var openSubStream;
    var onDataStream=null;
    var onDataStreamError=null;
    (function(){
      var reopen=false;
      var xhr=null;
      openSubStream=function(){
        if(reopen)return;
        if(xhr){
          xhr.abort();
          xhr=null;
          if(onDataStream||onJikkyoStream){
            reopen=true;
            setTimeout(function(){reopen=false;openSubStream();},5000);
          }
          return;
        }
        if(!onDataStream&&!onJikkyoStream)return;
        var readCount=0;
        var ctx={};
        xhr=new XMLHttpRequest();
        xhr.open("GET",document.getElementById("vidsrc").textContent+(onDataStream?"&psidata=1":"")+
                 (onJikkyoStream?"&jikkyo=1":"")+"&ofssec="+(ofssec+Math.floor(vid.currentTime*fast)));
        xhr.onloadend=function(){
          if(xhr&&(readCount==0||xhr.status!=0)){
            if(onDataStreamError)onDataStreamError(xhr.status,readCount);
            if(onJikkyoStreamError)onJikkyoStreamError(xhr.status,readCount);
          }
          xhr=null;
        };
        xhr.onprogress=function(){
          if(xhr&&xhr.status==200&&xhr.response){
            readCount=progressPsiDataChatMixedStream(readCount,xhr.response,onDataStream,onJikkyoStream,ctx);
          }
        };
        xhr.send();
      };
    })();
    if(useDatacast){
      var cbDatacast=document.getElementById("cb-datacast");
      cbDatacast.checked=false;
      cbDatacast.onclick=function(){
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
        onDataStream=function(pid,dict,code,pcr){
          dict[code]=bmlBrowserPlayTSSection(pid,dict[code],pcr)||dict[code];
        };
        onDataStreamError=function(status,readCount){
          document.querySelector(".remote-control-indicator").innerText="Error! ("+status+"|"+readCount+"Bytes)";
        };
        openSubStream();
      };
    }
    if(useLiveJikkyo||useJikkyoLog){
      var cbJikkyo=document.getElementById("cb-jikkyo");
      function onclickJikkyo(){
        if(!cbJikkyo.onclick&&vid.currentTime==0){
          setTimeout(onclickJikkyo,500);
          return;
        }
        cbJikkyo.onclick=onclickJikkyo;
        if(!cbJikkyo.checked){
          toggleJikkyo(false);
          openSubStream();
          setSendComment(null);
          document.querySelector('#vid-form input[name="jikkyo"]').value="0";
          return;
        }
        toggleJikkyo(true);
        if(useLiveJikkyo){
          setSendComment(function(value){
            var xhr=new XMLHttpRequest();
            xhr.open("POST","comment.lua");
            xhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
            xhr.onloadend=function(){
              if(xhr.status!=200){
                addMessage("Post error! ("+xhr.status+")");
              }
            };
            xhr.send(postCommentQuery+"&comm="+encodeURIComponent(value).replace(/%20/g,"+"));
          });
        }
        openSubStream();
        document.querySelector('#vid-form input[name="jikkyo"]').value="1";
      }
      setTimeout(onclickJikkyo,500);
    }
  }
  var voffset=document.getElementById("vid-offset");
  if(voffset){
    var vselect=document.querySelector('#vid-form select[name="offset"]');
    var vseek=document.getElementById("vid-seek");
    var vseekLeaved=true;
    var msList=[];
    for(var i=0;i<=100;i++){
      msList[i]=vselect.options[i].textContent.match(/^(?:\d+m\d\ds)?/)[0];
    }
    vseek.onmouseleave=function(){
      vseekLeaved=true;
      document.getElementById("vid-seek-status").innerText="";
    };
    vseek.oninput=function(){
      vseekLeaved=false;
      var sec=ofssec+Math.floor(vid.currentTime*fast);
      var ms=Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s";
      document.getElementById("vid-seek-status").innerText=ms+"\u2192"+msList[vseek.value]+"|"+vseek.value+"%";
    };
    vseek.onchange=function(){
      vselect.options[vseek.value].selected=true;
      document.querySelector('#vid-form button[type="submit"]').click();
    };
    vid.ontimeupdate=function(){
      var sec=ofssec+Math.floor(vid.currentTime*fast);
      var ms=Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s";
      voffset.innerText="|"+ms;
      if(vseekLeaved){
        for(var i=0;;i++){
          if(i==99||(msList[i]&&("000"+msList[i]).slice(-8)>=("000"+ms).slice(-8))){
            vseek.value=i;
            vseek.style.display=null;
            break;
          }
        }
      }
    };
    voffset.innerText="|"+Math.floor(ofssec/60)+"m"+String(100+ofssec%60).substring(1)+"s";
  }
}

function runHlsScript(aribb24UseSvg,aribb24Option,alwaysUseHls,hlsQuery,hlsMp4Query){
  var cap=null;
  var cbCaption=document.getElementById("cb-caption");
  function onclickCaption(){
    if(cbCaption.checked){
      if(!cap){
        aribb24Option.enableAutoInBandMetadataTextTrackDetection=!alwaysUseHls||!Hls.isSupported();
        cap=aribb24UseSvg?new aribb24js.SVGRenderer(aribb24Option):new aribb24js.CanvasRenderer(aribb24Option);
        cap.attachMedia(vid);
      }
      cap.show();
    }else if(cap){
      cap.hide();
    }
    document.querySelector('#vid-form input[name="caption"]').value=cbCaption.checked?"1":"0";
  }
  if(alwaysUseHls){
    onclickCaption();
    cbCaption.onclick=onclickCaption;
    document.getElementById("label-caption").style.display="inline";
    var cbLive=document.getElementById("cb-live");
    if(cbLive)cbLive.checked=true;
    vid.poster="loading.png";
    waitForHlsStart(document.getElementById("vidsrc").textContent+
      //Excludes Firefox for Android, because playback of non-keyframe fragmented MP4 is jerky.
      hlsQuery+(/Android.+Firefox/i.test(navigator.userAgent)?"":hlsMp4Query),1000,1000,function(){vid.poster=null;},function(src){
      if(Hls.isSupported()){
        var hls=new Hls();
        hls.loadSource(src);
        hls.attachMedia(vid);
        hls.on(Hls.Events.MANIFEST_PARSED,function(){vid.play();});
        hls.on(Hls.Events.FRAG_PARSING_METADATA,function(event,data){
          for(var i=0;cap&&i<data.samples.length;i++){cap.pushID3v2Data(data.samples[i].pts,data.samples[i].data);}
        });
        var vbitrate=document.getElementById("vid-bitrate");
        vbitrate.innerText="|?Mbps";
        var t=-1;
        var total=0;
        hls.on(Hls.Events.FRAG_BUFFERED,function(event,data){
          if(data.stats){
            var now=data.stats.buffering.end;
            if(t<0)t=now;
            else total+=data.stats.total;
            if(now-t>7000){
              vbitrate.innerText="|"+(total*1000/((now-t)*1024*128)).toFixed(1)+"Mbps";
              t=now;
              total=0;
            }
          }
        });
      }else if(vid.canPlayType("application/vnd.apple.mpegurl")){
        vid.src=src;
      }
    });
  }else{
    //Excludes Android even though canPlayType here may not return an empty string, because the quality of the native implementation is inconsistent.
    if(!/Android/i.test(navigator.userAgent)&&vid.canPlayType("application/vnd.apple.mpegurl")){
      onclickCaption();
      cbCaption.onclick=onclickCaption;
      document.getElementById("label-caption").style.display="inline";
      var cbLive=document.getElementById("cb-live");
      if(cbLive)cbLive.checked=true;
      vid.poster="loading.png";
      waitForHlsStart(document.getElementById("vidsrc").textContent+hlsQuery+hlsMp4Query,1000,1000,function(){vid.poster=null;},function(src){
        vid.src=src;
      });
    }else{
      vid.src=document.getElementById("vidsrc").textContent;
    }
  }
}
