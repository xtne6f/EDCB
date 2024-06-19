#include "stdafx.h"
#include "ConvertMacro2.h"

#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/EpgTimerUtil.h"

namespace
{
//マクロを展開した結果がこの長さを超えるときは失敗させる
const size_t MACRO_RESULT_LIMIT = 64 * 1024;

inline bool IsHighSurrogate(wchar_t c) { return L'\xD800' <= c && c <= L'\xDBFF'; }
inline bool IsLowSurrogate(wchar_t c) { return L'\xDC00' <= c && c <= L'\xDFFF'; }
}

wstring CConvertMacro2::Convert(const wstring& macro, const PLUGIN_RESERVE_INFO* info)
{
	wstring convert;

	for( size_t pos = 0;; ){
		size_t next = macro.find(L'$', pos);
		if( next == wstring::npos ){
			convert.append(macro, pos, wstring::npos);
			break;
		}
		convert.append(macro, pos, next - pos);
		pos = next;

		next = macro.find(L'$', pos + 1);
		if( next == wstring::npos ){
			convert.append(macro, pos, wstring::npos);
			break;
		}
		size_t brackets = macro.find(L"((", pos + 1);
		if( brackets < next ){
			//2重括弧の関数: $A(B((foo$C$)))$
			wstring trailer = wstring(std::count(macro.begin() + pos + 1, macro.begin() + brackets, L'(') + 2, L')') + L'$';
			next = macro.find(trailer, brackets);
			if( next == wstring::npos ){
				convert.append(macro, pos, wstring::npos);
				break;
			}
			next += trailer.size() - 1;
		}
		if( ExpandMacro(macro.substr(pos + 1, next - pos - 1), info, convert) == FALSE ){
			convert += L'$';
			pos++;
		}else{
			pos = next + 1;
		}
	}
	Replace(convert, L"\r", L"");
	Replace(convert, L"\n", L"");

	return convert;
}

BOOL CConvertMacro2::ExpandMacro(wstring var, const PLUGIN_RESERVE_INFO* info, wstring& convert)
{
	//関数を積む
	vector<wstring> funcStack;
	wstring ret;
	BOOL found = FALSE;
	while( !var.empty() && var.back() == L')' ){
		size_t n = var.find(L'(');
		if( n == wstring::npos ){
			return FALSE;
		}
		funcStack.push_back(var.substr(0, n));
		var = var.substr(n + 1, var.size() - 1 - (n + 1));
		if( !var.empty() && var[0] == L'(' ){
			if( var.back() != L')' ){
				return FALSE;
			}
			//2重括弧の関数は中身を展開
			ret = Convert(var.substr(1, var.size() - 2), info);
			found = TRUE;
			break;
		}
	}

	if( !found && (var.compare(0, 1, L"S") == 0 || var.compare(0, 1, L"E") == 0) ){
		for( int i = 0; GetTimeMacroName(i); i++ ){
			wstring name;
			UTF8toW(GetTimeMacroName(i), name);
			if( var.compare(1, wstring::npos, name) == 0 ){
				SYSTEMTIME st;
				//曜日フィールドが正しくない時代があったので必ず変換する
				ConvertSystemTime(ConvertI64Time(info->startTime) + (var[0] == L'S' ? 0 : info->durationSec) * I64_1SEC, &st);
				ret = GetTimeMacroValue(i, st);
				found = TRUE;
				break;
			}
		}
	}

	EPG_EVENT_INFO* epgInfo = info->epgInfo;
	if( found )	{}
	else if( var == L"Title" )	ret = info->eventName;
	else if( var == L"ONID10" )	Format(ret, L"%d", info->ONID);
	else if( var == L"TSID10" )	Format(ret, L"%d", info->TSID);
	else if( var == L"SID10" )	Format(ret, L"%d", info->SID);
	else if( var == L"EID10" )	Format(ret, L"%d", info->EventID);
	else if( var == L"ONID16" )	Format(ret, L"%04X", info->ONID);
	else if( var == L"TSID16" )	Format(ret, L"%04X", info->TSID);
	else if( var == L"SID16" )	Format(ret, L"%04X", info->SID);
	else if( var == L"EID16" )	Format(ret, L"%04X", info->EventID);
	else if( var == L"ServiceName" )	ret = info->serviceName;
	else if( var == L"DUHH" )	Format(ret, L"%02d", info->durationSec/(60*60));
	else if( var == L"DUH" )	Format(ret, L"%d", info->durationSec/(60*60));
	else if( var == L"DUMM" )	Format(ret, L"%02d", (info->durationSec%(60*60))/60);
	else if( var == L"DUM" )	Format(ret, L"%d", (info->durationSec%(60*60))/60);
	else if( var == L"DUSS" )	Format(ret, L"%02d", info->durationSec%60);
	else if( var == L"DUS" )	Format(ret, L"%d", info->durationSec%60);
	else if( var == L"BonDriverName" )	ret = info->bonDriverName;
	else if( var == L"BonDriverID" )	Format(ret, L"%d", info->bonDriverID);
	else if( var == L"TunerID" )	Format(ret, L"%d", info->tunerID);
	else if( var == L"ReserveID" )	Format(ret, L"%d", info->reserveID);
	else if( var == L"FreeCAFlag" )	Format(ret, L"%d", epgInfo ? epgInfo->freeCAFlag : -1);
	else if( var == L"Title2" ){
		ret = info->eventName;
		while( ret.find(L"[") != wstring::npos && ret.find(L"]") != wstring::npos ){
			wstring strSep1;
			wstring strSep2;
			Separate(ret, L"[", ret, strSep1);
			Separate(strSep1, L"]", strSep2, strSep1);
			ret += strSep1;
		}
	}else if( var == L"Genre" ){
		if( epgInfo != NULL && epgInfo->contentInfo != NULL && epgInfo->contentInfo->listSize > 0 ){
			BYTE nibble1 = epgInfo->contentInfo->nibbleList[0].content_nibble_level_1;
			BYTE nibble2 = epgInfo->contentInfo->nibbleList[0].content_nibble_level_2;
			if( nibble1 == 0x0E && nibble2 <= 0x01 ){
				//番組付属情報またはCS拡張用情報
				nibble1 = epgInfo->contentInfo->nibbleList[0].user_nibble_1 | (0x60 + nibble2 * 16);
			}
			ret = GetGenreName(nibble1, 0xFF);
			if( ret.empty() ){
				Format(ret, L"(0x%02X)", nibble1);
			}
		}
	}else if( var == L"Genre2" ){
		if( epgInfo != NULL && epgInfo->contentInfo != NULL && epgInfo->contentInfo->listSize > 0 ){
			BYTE nibble1 = epgInfo->contentInfo->nibbleList[0].content_nibble_level_1;
			BYTE nibble2 = epgInfo->contentInfo->nibbleList[0].content_nibble_level_2;
			if( nibble1 == 0x0E && nibble2 <= 0x01 ){
				//番組付属情報またはCS拡張用情報
				nibble1 = epgInfo->contentInfo->nibbleList[0].user_nibble_1 | (0x60 + nibble2 * 16);
				nibble2 = epgInfo->contentInfo->nibbleList[0].user_nibble_2;
			}
			ret = GetGenreName(nibble1, nibble2);
			if( ret.empty() && nibble1 != 0x0F ){
				Format(ret, L"(0x%02X)", nibble2);
			}
		}
	}else if( var == L"SubTitle" ){
		if( epgInfo != NULL && epgInfo->shortInfo != NULL ){
			ret = epgInfo->shortInfo->text_char;
		}
	}else if( var == L"SubTitle2" ){
		if( epgInfo != NULL && epgInfo->shortInfo != NULL ){
			wstring strSubTitle2 = epgInfo->shortInfo->text_char;
			strSubTitle2 = strSubTitle2.substr(0, strSubTitle2.find(L"\r\n"));
			LPCWSTR startsWith[] = { L"#＃第", L"0123456789０１２３４５６７８９", NULL };
			for( size_t j, i = 0; i < strSubTitle2.size(); i++ ){
				for( j = 0; startsWith[i][j] && startsWith[i][j] != strSubTitle2[i]; j++ );
				if( startsWith[i][j] == L'\0' ){
					break;
				}
				if( startsWith[i+1] == NULL ){
					ret = strSubTitle2;
					break;
				}
			}
		}
	}else if( var == L"ExtEventInfo" ){
		if( epgInfo && epgInfo->extInfo ){
			ret = epgInfo->extInfo->text_char;
		}
	}else{
		return FALSE;
	}

	//関数を適用
	while( !funcStack.empty() ){
		wstring func = funcStack.back();
		funcStack.pop_back();
		for( size_t i = 0; i < func.size(); i++ ){
			//数値文字参照(&文字コード;)を展開
			if( func[i] == L'&' ){
				wchar_t* p;
				wchar_t c = (wchar_t)wcstol(&func.c_str()[i + 1], &p, 10);
				func.replace(i, p - func.c_str() - i + (*p ? 1 : 0), 1, c);
			}
		}
		if( func == L"HtoZ" ){
			funcStack.push_back(L"Tr＼ !\"#&36;%&38;'&40;)*+,-./:;<=>?@[\\]^_`{|}~＼　！”＃＄％＆’（）＊＋，－．／：；＜＝＞？＠［￥］＾＿‘｛｜｝￣＼");
			funcStack.push_back(L"HtoZ<alnum>");
		}else if( func == L"ZtoH" ){
			funcStack.push_back(L"Tr＼　！”＃＄％＆’（）＊＋，－．／：；＜＝＞？＠［￥］＾＿‘｛｜｝￣＼ !\"#&36;%&38;'&40;)*+,-./:;<=>?@[\\]^_`{|}~＼");
			funcStack.push_back(L"ZtoH<alnum>");
		}else if( func == L"HtoZ<alnum>" ){
			funcStack.push_back(L"Tr/0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ/");
		}else if( func == L"ZtoH<alnum>" ){
			funcStack.push_back(L"Tr/０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ/0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/");
		}else if( func == L"ToSJIS" ){
			string retA;
			WtoA(ret, retA);
			AtoW(retA, ret);
		}else if( func.compare(0, 2, L"Tr") == 0 && func.size() >= 3 ){
			//文字置換(Tr/置換文字リスト/置換後/)
			size_t n = func.find(func[2], 3);
			size_t m;
			if( n == wstring::npos || (m = func.find(func[2], n + 1)) == wstring::npos ){
				return FALSE;
			}
			wstring cmp;
			cmp.reserve((n - 3) * 2);
			for( size_t i = 3; i < n; i++ ){
				//非サロゲートペアをダブらせて文字ごとに固定長にする
				cmp.append(IsHighSurrogate(func[i]) || IsLowSurrogate(func[i]) ? 1 : 2, func[i]);
			}
			size_t rpos = 0;
			for( size_t i = n + 1; i < m; i++ ){
				rpos += IsHighSurrogate(func[i]) || IsLowSurrogate(func[i]) ? 1 : 2;
			}
			if( rpos != cmp.size() ){
				//リストの文字数が合わない
				return FALSE;
			}
			for( size_t i = 0; i < ret.size(); ){
				size_t inc = 1;
				size_t pos = wstring::npos;
				if( IsHighSurrogate(ret[i]) && i + 1 < ret.size() ){
					inc = 2;
					pos = cmp.find(ret.c_str() + i, 0, 2);
				}else if( !IsHighSurrogate(ret[i]) && !IsLowSurrogate(ret[i]) ){
					pos = cmp.find(ret[i]);
				}
				if( pos != wstring::npos ){
					size_t j = n + 1;
					for( rpos = 0; j < m && rpos < pos; j++ ){
						rpos += IsHighSurrogate(func[j]) || IsLowSurrogate(func[j]) ? 1 : 2;
					}
					if( j >= m || (IsHighSurrogate(func[j]) && j + 1 >= m) ){
						return FALSE;
					}
					ret.replace(i, inc, func, j, IsHighSurrogate(func[j]) ? 2 : 1);
					if( ret.size() > MACRO_RESULT_LIMIT ){
						return FALSE;
					}
					inc = IsHighSurrogate(func[j]) ? 2 : 1;
				}
				i += inc;
			}
		}else if( func.compare(0, 1, L"S") == 0 && func.size() >= 2 ){
			//文字列置換(S/置換文字列/置換後/.../)
			for( size_t i = 0; i < ret.size(); ){
				for( size_t j = 2;; ){
					size_t n = func.find(func[1], j);
					if( n == wstring::npos ){
						i++;
						break;
					}
					size_t m = func.find(func[1], n + 1);
					if( m == wstring::npos ){
						return FALSE;
					}
					if( n > j && ret.compare(i, n - j, func, j, n - j) == 0 ){
						ret.replace(i, n - j, func, n + 1, m - (n + 1));
						if( ret.size() > MACRO_RESULT_LIMIT ){
							return FALSE;
						}
						i += m - (n + 1);
						break;
					}
					j = m + 1;
				}
			}
		}else if( func.compare(0, 2, L"Rm") == 0 && func.size() >= 3 ){
			//文字削除(Rm/削除文字リスト/)
			size_t n = func.find(func[2], 3);
			if( n == wstring::npos ){
				return FALSE;
			}
			wstring cmp(func, 3, n - 3);
			for( size_t i = 0; i < ret.size(); ){
				if( IsHighSurrogate(ret[i]) && i + 1 < ret.size() ){
					if( cmp.find(ret.c_str() + i, 0, 2) != wstring::npos ){
						ret.erase(i, 2);
					}else{
						i += 2;
					}
				}else if( !IsHighSurrogate(ret[i]) && !IsLowSurrogate(ret[i]) && cmp.find(ret[i]) != wstring::npos ){
					ret.erase(i, 1);
				}else{
					i++;
				}
			}
		}else if( func.compare(0, 4, L"Head") == 0 && func.size() >= 5 ){
			//足切り(Head[C]文字数[省略記号])
			bool isC = func[4] == L'C';
			wchar_t* p;
			size_t m = (size_t)wcstol(&func.c_str()[4 + isC], &p, 10);
			bool hasSpace = false;
			if( isC ){
				//UTF-8相当の長さをwchar_tの長さに変換
				size_t i = 0;
				for( size_t j = 0; i < ret.size(); i++ ){
					hasSpace = j < m;
					if( IsHighSurrogate(ret[i]) && i + 1 < ret.size() && IsLowSurrogate(ret[i + 1]) ){
						j += 4;
						if( j > m ){
							break;
						}
						i++;
					}else{
						char dest[4];
						j += codepoint_to_utf8(ret[i], dest);
						if( j > m ){
							break;
						}
					}
				}
				m = i;
			}
#if WCHAR_MAX > 0xFFFF
			else{
				//UTF-16相当の長さをwchar_tの長さに変換
				for( size_t i = 0; i < m && i < ret.size(); i++ ){
					if( ret[i] > L'\xFFFF' ){
						--m;
					}
					hasSpace = m == i;
				}
			}
#endif
			if( m < ret.size() ){
				if( *p && (m > 0 || hasSpace) ){
					if( !hasSpace ){
						m--;
					}
				}else{
					p = NULL;
				}
				if( m > 0 && IsHighSurrogate(ret[m - 1]) ){
					m--;
				}
				ret.erase(m);
				if( p ){
					ret.push_back(*p);
				}
			}
		}else if( func.compare(0, 4, L"Tail") == 0 && func.size() >= 5 ){
			//頭切り(Tail[C]文字数[省略記号])
			bool isC = func[4] == L'C';
			wchar_t* p;
			size_t m = (size_t)wcstol(&func.c_str()[4 + isC], &p, 10);
			bool hasSpace = false;
			if( isC ){
				//UTF-8相当の長さをwchar_tの長さに変換
				size_t i = 0;
				for( size_t j = 0; i < ret.size(); i++ ){
					hasSpace = j < m;
					if( IsLowSurrogate(ret[ret.size() - 1 - i]) && i + 1 < ret.size() && IsHighSurrogate(ret[ret.size() - 2 - i]) ){
						j += 4;
						if( j > m ){
							break;
						}
						i++;
					}else{
						char dest[4];
						j += codepoint_to_utf8(ret[ret.size() - 1 - i], dest);
						if( j > m ){
							break;
						}
					}
				}
				m = i;
			}
#if WCHAR_MAX > 0xFFFF
			else{
				//UTF-16相当の長さをwchar_tの長さに変換
				for( size_t i = 0; i < m && i < ret.size(); i++ ){
					if( ret[ret.size() - 1 - i] > L'\xFFFF' ){
						--m;
					}
					hasSpace = m == i;
				}
			}
#endif
			if( m < ret.size() ){
				if( *p && (m > 0 || hasSpace) ){
					if( !hasSpace ){
						m--;
					}
				}else{
					p = NULL;
				}
				if( m > 0 && IsLowSurrogate(ret[ret.size() - m]) ){
					m--;
				}
				ret.erase(0, ret.size() - m);
				if( p ){
					ret.insert(ret.begin(), *p);
				}
			}
		}else{
			return FALSE;
		}
	}

	if( convert.size() + ret.size() > MACRO_RESULT_LIMIT ){
		return FALSE;
	}
	convert += ret;

	return TRUE;
}
