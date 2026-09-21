"use strict";
//jshint browser: true, esversion: 3

var base64ToUint8Array=window.Uint8Array&&(Uint8Array.fromBase64?function(a){return Uint8Array.fromBase64(a);}:function(a){
  var b=atob(a);
  var u=new Uint8Array(b.length);
  for(var i=0;i<b.length;i++)u[i]=b.charCodeAt(i);
  return u;
});

if(window.addEventListener)window.addEventListener("DOMContentLoaded",function(){
  var unloadCount=0;
  function createUnloaded(){
    var c=unloadCount;
    return function(){return c<unloadCount;};
  }

  if(window.createMiscWasmModule){
    (function(){
      var cachedMod=null;
      var createMod=createMiscWasmModule;
      createMiscWasmModule=function(){
        return new Promise(function(res,rej){
          if(cachedMod)res(cachedMod);
          else createMod().then(function(mod){res(cachedMod=mod);},rej);
        });
      };
    })();
  }

  function runCCInfoScript(){
    var unloaded=createUnloaded();
    var hw=" ,.:;?!^_/|()[]{}+-=<>$%#&*@";
    var fw="\u3000\uff0c\uff0e\uff1a\uff1b\uff1f\uff01\uff3e\uff3f\uff0f\uff5c\uff08\uff09\uff3b\uff3d\uff5b\uff5d\uff0b\uff0d\uff1d\uff1c\uff1e\uff04\uff05\uff03\uff06\uff0a\uff20";
    var reFull=new RegExp("["+fw+"\uff10-\uff19\uff21-\uff3a\uff41-\uff5a]","g");
    function toHalf(s){
      return s.replace(reFull,function(c){
        //[0-9A-Za-z]
        return /[\uff10-\uff19\uff21-\uff3a\uff41-\uff5a]/.test(c)?String.fromCharCode(c.charCodeAt(0)-0xfee0):hw[fw.indexOf(c)];
      });
    }
    function extractReadableText(s,b24,f){
      var ctrl=0;
      var ssz=false;
      for(var i=0;i<s.length;i++){
        if(s[i]=="<"){
          //Ignore tags
          i++;
          if(s[i]=="c"&&(s[i+1]==" "||s[i+1]==">")){
            //<c>
            ctrl++;
          }else if(s[i]=="/"&&s[i+1]=="c"&&(s[i+2]==" "||s[i+2]==">")){
            //</c>
            ctrl--;
          }
          i=s.indexOf(">",i);
          if(i<0)i=s.length-1;
        }else if(!b24||ctrl<=0){
          //readable
          var j=s.indexOf("<",i);
          if(j<0)j=s.length;
          var t=s.substring(i,j).replace(/&(?:amp|lt|gt|quot|apos);/g,function(m){
            return m=="&amp;"?"&":m=="&lt;"?"<":m=="&gt;"?">":m=="&quot;"?'"':"'";
          });
          t=toHalf(t);
          //TODO: Draw DRCS
          t=t.replace(/[\uec00-\uefff]/g,"");
          if(t)f(t,ssz);
          i=j-1;
        }else if(s[i]=="%"&&s[i+1]=="^"&&s[i+2]=="H"){
          //SSZ
          ssz=true;
        }else if(s[i]=="%"&&s[i+1]=="^"&&(s[i+2]=="I"||s[i+2]=="J"||s[i+2]=="K")){
          //MSZ,NSZ,SZX
          ssz=false;
        }
      }
    }

    var cb=document.getElementById("cb-ccinfo");
    if(!cb)return;
    cb.checked=false;
    cb.parentElement.parentElement.style.display=null;
    var dummyVideo=null;
    cb.onchange=function(){
      if(unloaded())return;
      var desc=document.getElementById("ccinfo-desc");
      desc.style.display=cb.checked?null:"none";
      if(!cb.checked||dummyVideo)return;
      dummyVideo=document.createElement("video");
      desc.innerText="Loading...";
      function parseCue(uri,b24,download){
        var track=document.createElement("track");
        dummyVideo.appendChild(track);
        track.kind="metadata";
        track.src=uri;
        track.type="text/vtt";
        track=track.track;
        track.mode="hidden";
        var to=50;
        var waitForCue=function(){
          if(unloaded())return;
          if(track.cues.length==0){
            if(--to<0){
              desc.innerText="Error!";
              dummyVideo=null;
            }else{
              setTimeout(waitForCue,100);
            }
            return;
          }
          if(desc.dataset.setSrc){
            //Set object URL
            document.getElementById(desc.dataset.setSrc).src=uri;
          }
          var a=document.createElement("a");
          a.download=download;
          a.href=uri;
          a.innerText="(DL)";
          desc.innerText="";
          desc.parentElement.insertBefore(a,desc);

          var re=b24?/<v b24caption[1-8]>(.*?)<\/v>/g:/(.+)/g;
          for(var i=0;i<track.cues.length;i++){
            re.lastIndex=0;
            var cueText=track.cues[i].text.replace(/\r?\n/g,b24?"":" ");
            var src;
            while((src=re.exec(cueText))!==null){
              var appended=false;
              extractReadableText(src[1],b24,function(s,ssz){
                if(!appended){
                  appended=true;
                  var sec=track.cues[i].startTime;
                  var a=document.createElement("a");
                  if(desc.dataset.seekTarget){
                    if(desc.dataset.seekTarget=="vid-form"){
                      a.onclick=function(){
                        var selectOfssec=document.querySelector('#vid-form select[name="ofssec"]');
                        if(selectOfssec){
                          for(var i=1;;i++){
                            if(i==100||selectOfssec.options[selectOfssec.options.length-101+i].value>=Math.max(sec-1,0)){
                              selectOfssec.options[selectOfssec.options.length-102+i].selected=true;
                              break;
                            }
                          }
                        }
                      };
                    }else{
                      a.onclick=function(e){
                        seekVideo(Math.max(sec-1,0));
                        if(unloadCount>0){
                          e.preventDefault();
                          window.scrollTo(0,window.scrollY+document.getElementById(desc.dataset.seekTarget).getBoundingClientRect().top);
                        }
                      };
                    }
                    a.href="#"+desc.dataset.seekTarget;
                  }
                  a.innerText=(sec<600?"0":"")+Math.floor(sec/60)+"m"+String(Math.floor(100+sec%60)).substring(1)+"s";
                  desc.appendChild(a);
                  desc.appendChild(document.createTextNode(" "));
                }
                if(ssz){
                  var small=document.createElement("small");
                  small.innerText=s;
                  desc.appendChild(small);
                }else{
                  desc.appendChild(document.createTextNode(s));
                }
              });
              if(appended)desc.appendChild(document.createElement("br"));
            }
          }
        };
        waitForCue();
      }
      if(desc.dataset.fname){
        var xhr=new XMLHttpRequest();
        xhr.open("GET","vtt.lua?fname="+encodeURIComponent(desc.dataset.fname));
        xhr.onloadend=function(){
          if(xhr.status==200&&xhr.response){
            var s=xhr.response.replace(/\r?\n/g,"\n").replace(/\nNOTE msec=[0-9]{8}\n/g,"");
            //Validate
            if(/^WEBVTT\n[\s\S]* --> /.test(s)){
              var m=(xhr.getResponseHeader("Content-Disposition")||"").match(/filename=([0-9A-Za-z_-]+\.vtt)/);
              parseCue(URL.createObjectURL(new Blob(["\ufeff",s],{endings:"native",type:"text/vtt"})),true,m?m[1]:"a.vtt");
            }else{
              desc.innerText="";
            }
          }else{
            desc.innerText="Error! ("+xhr.status+")";
            dummyVideo=null;
          }
        };
        xhr.onprogress=function(){
          if(xhr.status==200&&xhr.response){
            var s=xhr.response.replace(/\r?\n/g,"\n");
            var i=s.lastIndexOf("\n\nNOTE msec=",s.length-20);
            if(i>=0){
              var sec=Math.floor(s.substring(i+12,i+20)/1000);
              var count=0;
              for(i=0;(i=s.indexOf(" --> ",i+5))>=0;count++);
              desc.innerText="Loading... ("+Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s|count="+count+")";
            }
          }
          if(unloaded())xhr.abort();
        };
        xhr.send();
      }else{
        parseCue(encodeURIComponent(desc.dataset.vtt).replace(/%2F/g,"/"),desc.dataset.vttKind=="metadata","");
      }
    };
  }

  function runThumbnailScript(){
    var unloaded=createUnloaded();
    var thumbs=document.getElementById("vid-thumbs");
    if(!thumbs||!window.createMiscWasmModule)return;
    try{
      createMiscWasmModule().then(function(mod){
        if(unloaded())return;
        var flipTimer=0;
        var pushed=null;
        var div=document.createElement("div");
        div.style.display="none";
        thumbs.appendChild(div);
        var streams=JSON.parse(document.getElementById("vid-thumb-streams").text);
        for(var i=0;i<streams.length;i++){
          var u=base64ToUint8Array(streams[i][0]);
          var buffer=mod.getGrabberInputBuffer(u.length);
          buffer.set(u);
          var frame=mod.grabFirstFrame(u.length);
          if(!frame)continue;
          (function(){
            var canvas=document.createElement("canvas");
            if(thumbs.dataset.fname){
              var sec=streams[i][1];
              function flip(){
                var myTimer=flipTimer;
                var xhr=new XMLHttpRequest();
                xhr.open("GET","grabber.lua?fname="+encodeURIComponent(thumbs.dataset.fname)+"&ofssec="+(sec+5));
                xhr.responseType="arraybuffer";
                xhr.onloadend=function(){
                  if(unloaded())return;
                  if(xhr.status!=200||!xhr.response){
                    if(flipTimer==myTimer)flipTimer=setTimeout(flip,3000);
                    return;
                  }
                  var buffer=mod.getGrabberInputBuffer(xhr.response.byteLength);
                  buffer.set(new Uint8Array(xhr.response));
                  var frame=mod.grabFirstFrame(xhr.response.byteLength);
                  if(frame){
                    canvas.width=frame.width;
                    canvas.height=frame.height;
                    canvas.getContext("2d").putImageData(new ImageData(new Uint8ClampedArray(frame.buffer),frame.width,frame.height),0,0);
                  }
                  sec+=5;
                  if(flipTimer==myTimer){
                    div.innerText=Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s";
                    flipTimer=setTimeout(flip,500);
                  }
                };
                xhr.send();
              }
              canvas.onmouseenter=function(){
                var ra=canvas.getBoundingClientRect();
                var rb=thumbs.getBoundingClientRect();
                div.style.left=ra.x-rb.x+"px";
                div.style.bottom=rb.bottom-ra.bottom+"px";
                div.innerText=Math.floor(sec/60)+"m"+String(100+sec%60).substring(1)+"s";
                div.style.display=null;
                clearTimeout(flipTimer);
                flipTimer=setTimeout(flip,1000);
              };
              canvas.onmouseleave=function(){
                clearTimeout(flipTimer);
                flipTimer=0;
                div.style.display="none";
                pushed=null;
              };
              canvas.onclick=function(){
                pushed=pushed==canvas?null:canvas;
                if(pushed)canvas.onmouseenter();
                else canvas.onmouseleave();
              };
            }
            canvas.width=frame.width;
            canvas.height=frame.height;
            canvas.getContext("2d").putImageData(new ImageData(new Uint8ClampedArray(frame.buffer),frame.width,frame.height),0,0);
            document.getElementsByClassName("thumb-placeholder")[i].appendChild(canvas);
          })();
        }
      });
    }catch(e){console.warn("createMiscWasmModule():",e);}
  }

  function runTransitInplaceScript(onLoaded){
    if(!window.URLSearchParams)return;
    var body=document.getElementsByTagName("body")[0];
    function transit(method,url){
      var xhr=new XMLHttpRequest();
      xhr.open(method,url);
      xhr.onloadend=function(){
        var m=xhr.status==200&&xhr.responseText.match(/<body>([\s\S]*)<\/body>/);
        if(!m){
          if(method.toUpperCase()=="GET")location.href=url.href;
          return;
        }
        var loaded={};
        body.querySelectorAll("script").forEach(function(s){
          var src=s.getAttribute("src");
          if(src)loaded[src]=true;
        });
        unloadCount++;
        history.pushState(null,"",url.href);
        onpopstate=function(){history.go();};
        body.innerHTML=m[1];
        var loading={};
        body.querySelectorAll("script").forEach(function(s){
          var src=s.getAttribute("src");
          if(src&&!loaded[src]){
            s=document.createElement("script");
            s.src=src;
            loading[src]=true;
            s.onerror=s.onload=function(){
              if(loading[src]){
                delete loading[src];
                if(Object.keys(loading).length==0){
                  onLoaded();
                  window.dispatchEvent(new Event("my-load"));
                }
              }
            };
            body.appendChild(s);
          }
        });
      };
      xhr.send();
    }
    body.querySelectorAll(".transit-inplace").forEach(function(a){
      if(a.href){
        a.onclick=function(e){
          e.preventDefault();
          transit("GET",new URL(a.href));
        };
      }else{
        a.onsubmit=function(e){
          e.preventDefault();
          var url=new URL(a.action);
          url.search=new URLSearchParams(new FormData(a));
          transit(a.method,url);
        };
      }
    });
  }

  function onAboutLoaded(){
    document.getElementById("env-info").innerHTML=window.isSecureContext?", secure_context":", <s>secure_context</s>";
    var initDate=new Date(document.getElementById("env-date").innerText+"Z");
    var initNow=Date.now();
    function addDate(){
      var d=new Date(Date.now()-initNow+initDate.getTime());
      document.getElementById("env-date").innerText=d.toISOString().substring(0,19);
      setTimeout(addDate,10050-d.getTime()%10000);
    }
    addDate();
  }

  function onAutoaddepginfoLoaded(){
    document.querySelector("select[name=contentList]").onchange=function(){
      var selList=[];
      var options=document.querySelectorAll("select[name=contentList] option");
      var nibble1="";
      for(var i=0;i<options.length;i++){
        var s=options[i].textContent;
        nibble1=s[0]=="\u3000"?nibble1:s;
        if(options[i].selected){
          selList.push((s==nibble1?"":"("+nibble1.trim()+")")+s.trim());
        }
      }
      var note=document.getElementById("content-note");
      note.textContent=selList.slice(0,10).join("; ")+(selList.length>10?"; "+note.dataset.other.replace("%",selList.length-10):"");
    };
    document.querySelector("select[name=serviceList]").onchange=function(){
      var selList=[];
      var options=document.querySelectorAll("select[name=serviceList] option");
      for(var i=0;i<options.length;i++){
        if(options[i].selected){
          selList.push(options[i].textContent.trim());
        }
      }
      var note=document.getElementById("service-note");
      note.textContent=selList.slice(0,10).join("; ")+(selList.length>10?"; "+note.dataset.other.replace("%",selList.length-10):"");
    };
    var timeOrWeek=document.querySelectorAll("input[name=timeOrWeek]");
    var timePeriod;
    function onchangeTimeOrWeek(){
      timePeriod=timeOrWeek[0].checked==(timeOrWeek[0].value=="1");
      document.getElementById("check-week").style=timePeriod?"display:none":"";
      document.querySelector("select[name=startDayOfWeek]").style=timePeriod?"":"display:none";
      document.querySelector("select[name=endDayOfWeek]").style=timePeriod?"":"display:none";
    }
    timeOrWeek[0].onchange=onchangeTimeOrWeek;
    timeOrWeek[1].onchange=onchangeTimeOrWeek;
    onchangeTimeOrWeek();
    document.querySelector("#add-time-or-week button").onclick=function(){
      var st=document.querySelector("input[name=startTime]").value.match(/(\d+):(\d+)/);
      var et=document.querySelector("input[name=endTime]").value.match(/(\d+):(\d+)/);
      if(!st||!et)return;
      st=Math.min(+st[2]+st[1]*60,24*60-1);
      et=Math.min(+et[2]+et[1]*60,24*60-1);
      var s="";
      if(timePeriod){
        s=document.querySelector("select[name=startDayOfWeek]").value+Math.floor(st/60)+":"+(st%60)+"-"+
          document.querySelector("select[name=endDayOfWeek]").value+Math.floor(et/60)+":"+(et%60);
      }else{
        //For each day of week
        for(var i=0;i<7;i++){
          if(document.querySelector("input[name=checkDayOfWeek"+i+"]").checked){
            s+=(s==""?"":", ")+(["sun","mon","tue","wed","thu","fri","sat"])[i]+Math.floor(st/60)+":"+(st%60)+"-"+
               (["sun","mon","tue","wed","thu","fri","sat"])[st>et?(i+1)%7:i]+Math.floor(et/60)+":"+(et%60);
          }
        }
      }
      if(s=="")return;
      var dateList=document.querySelector("input[name=dateList]");
      dateList.value+=(dateList.value==""?"":", ")+s;
    };
    document.getElementById("add-time-or-week").style="";
  }

  function onEpginfoLoaded(){
    var desc=document.getElementById("pginfo-desc");
    if(desc&&desc.textContent){
      var a=document.createElement("a");
      a.download="a.program.txt";
      a.href=URL.createObjectURL(new Blob(["\ufeff",desc.textContent],{endings:"native",type:"text/plain"}));
      a.innerText="(DL)";
      document.getElementById("pginfo-term").appendChild(a);
    }
  }

  function onEpglistLoaded(){
    var initTexts=null;
    var lastChecked=null;
    var sel=document.querySelector("select[name=id]");
    if(sel){
      sel.ontouchstart=sel.onfocus=sel.onmouseover=function(){
        //Dynamically get the current event name
        if(lastChecked&&Math.abs(Date.now()-lastChecked)<30000)return;
        lastChecked=Date.now();
        var xhr=new XMLHttpRequest();
        xhr.open("GET","epglist.html");
        xhr.onload=function(){
          if(xhr.status!=200||!xhr.response)return;
          var optionsDict={};
          for(var i=0;i<sel.options.length;i++){
            optionsDict[sel.options[i].value]=sel.options[i];
          }
          if(!initTexts){
            initTexts={};
            for(var i=0;i<sel.options.length;i++){
              initTexts[sel.options[i].value]=sel.options[i].innerHTML;
            }
            sel.style.width=sel.offsetWidth+"px";
          }
          var re=/<a href="epglist\.html\?id=(\d+-\d+-\d+)#now".*?<\/a><span[^>]*>.*?<\/span>([^<>]*)<\/span>/g;
          var m;
          while((m=re.exec(xhr.response))!==null){
            if(m[1] in initTexts){
              optionsDict[m[1]].innerHTML=initTexts[m[1]]+"- "+m[2];
            }
          }
        };
        xhr.send();
      };
    }
    runTransitInplaceScript(onEpglistLoaded);
    if(document.getElementById("video"))runPlaybackScript(unloadCount>0);
  }

  function onLibraryLoaded(){
    var cb=document.getElementById("cb-pginfo");
    if(cb){
      cb.checked=false;
      cb.parentElement.parentElement.style.display=null;
      var xhr=null;
      cb.onchange=function(){
        var desc=document.getElementById("pginfo-desc");
        desc.style.display=cb.checked?null:"none";
        if(!cb.checked||xhr)return;
        xhr=new XMLHttpRequest();
        xhr.open("GET","pgtxt.lua?fname="+encodeURIComponent(desc.dataset.fname));
        xhr.onloadend=function(){
          if(xhr.status!=200||!xhr.responseText){
            desc.innerText="Error! ("+xhr.status+")";
            return;
          }
          desc.innerHTML=xhr.responseText;
          var a=document.createElement("a");
          var m=(xhr.getResponseHeader("Content-Disposition")||"").match(/filename=([0-9A-Za-z_-]+\.program\.txt)/);
          a.download=m?m[1]:"a.program.txt";
          a.href=URL.createObjectURL(new Blob(["\ufeff",desc.textContent],{endings:"native",type:"text/plain"}));
          a.innerText="(DL)";
          desc.parentElement.insertBefore(a,desc);
        };
        xhr.send();
      };
    }
    runCCInfoScript();
    runThumbnailScript();
    runTransitInplaceScript(onLibraryLoaded);
    if(document.getElementById("video"))runPlaybackScript(unloadCount>0);
  }

  function onNvramLoaded(){
    var prefix="nvram_prefix=receiverinfo%2F";
    var zip=document.getElementById("nvram-zip");
    var region=document.getElementById("nvram-region");
    var v=localStorage.getItem(prefix+"zipcode");
    if(v){
      try{
        v=atob(v);
        if(/^[0-9]{7}$/.test(v))zip.value=v;
      }catch(e){}
    }else{
      zip.value=zip.dataset.absent;
    }
    v=localStorage.getItem(prefix+"regioncode");
    if(v){
      try{
        v=atob(v);
        v=v.charCodeAt(0)<<8|v.charCodeAt(1);
        for(var i=0;i<region.options.length;i++){
          if(parseInt(region.options[i].value,16)===v){
            region.options[i].selected=true;
            break;
          }
        }
      }catch(e){}
    }else{
      region.options[region.dataset.absent].selected=true;
    }
    document.getElementById("nvram-gov").onclick=function(){
      if(region.selectedIndex>0){
        zip.value=region.options[region.selectedIndex].dataset.zip;
      }
    };
    var btnSet=document.getElementById("nvram-set");
    btnSet.onclick=function(){
      if(/^[0-9]{7}$/.test(zip.value)){
        localStorage.setItem(prefix+"zipcode",btoa(zip.value));
      }else{
        localStorage.removeItem(prefix+"zipcode");
      }
      if(region.selectedIndex>0){
        localStorage.setItem(prefix+"prefecture",btoa(String.fromCharCode(region.selectedIndex)));
        localStorage.setItem(prefix+"regioncode",btoa(String.fromCharCode(parseInt(region.value,16)>>8,parseInt(region.value,16)&0xff)));
      }else{
        localStorage.removeItem(prefix+"prefecture");
        localStorage.removeItem(prefix+"regioncode");
      }
      document.getElementById("result").innerText=btnSet.dataset.result;
    };
    var btnDel=document.getElementById("nvram-del");
    btnDel.onclick=function(){
      var keys=[];
      for(var i=0;i<localStorage.length;i++){
        if(/^nvram_/.test(localStorage.key(i))){
          keys.push(localStorage.key(i));
        }
      }
      for(var i=0;i<keys.length;i++){
        localStorage.removeItem(keys[i]);
      }
      document.getElementById("result").innerText=btnDel.dataset.result;
    };
    var cb=document.getElementById("use-js-interpreter");
    cb.checked=!!localStorage.getItem("use_js_interpreter");
    cb.onchange=function(){
      if(cb.checked)localStorage.setItem("use_js_interpreter","true");
      else localStorage.removeItem("use_js_interpreter");
      document.getElementById("result").innerText=cb.dataset.result.replace(/%(.*?)%(.*?)%/,localStorage.getItem("use_js_interpreter")?"$2":"$1");
    };
  }

  function onRecinfodescReserveinfoLoaded(){
    onEpginfoLoaded();
    runCCInfoScript();
    runThumbnailScript();
    runTransitInplaceScript(onRecinfodescReserveinfoLoaded);
    if(document.getElementById("video"))runPlaybackScript(unloadCount>0);
  }

  function onReserveLoaded(){
    runTransitInplaceScript(onReserveLoaded);
    if(document.getElementById("video"))runPlaybackScript(unloadCount>0);
  }

  switch(document.getElementById("common-js").getAttribute("data-script-name")){
    case "about.html":
      onAboutLoaded();
      break;
    case "autoaddepginfo.html":
      onAutoaddepginfoLoaded();
      break;
    case "epginfo.html":
    case "epgpastinfo.html":
      onEpginfoLoaded();
      break;
    case "epglist.html":
      onEpglistLoaded();
      break;
    case "library.html":
      onLibraryLoaded();
      break;
    case "nvram.html":
      onNvramLoaded();
      break;
    case "recinfodesc.html":
    case "reserveinfo.html":
      onRecinfodescReserveinfoLoaded();
      break;
    case "reserve.html":
      onReserveLoaded();
      break;
  }
});
