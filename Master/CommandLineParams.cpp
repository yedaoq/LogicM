#include "stdafx.h"
#include "CommandLineParams.h"
#include "StringUtil.h"

bool is_parakey_end(tchar c)
{
	return StringUtil::is_blank_char(c) || ':' == c || '=' == c;
}
// 
// const tchar* get_para_block_end(const tchar* head, bool (*match_end)(tchar), bool head_blank)
// {
// 	if(head_blank)								// ȥ����ʼ���Ŀո�
// 	{
// 		if(0 == (head = StringUtil::find(head, StringUtil::not_blank_char<tchar>)))
// 			return 0;
// 	}
// 
// 	for (const tchar* tmp = head; *tmp; ++tmp)	// �����ַ���ֱ������������ַ��������е��ַ����⣩
// 	{
// 		if('"' == *tmp || '\'' == *tmp)
// 		{
// 			if(0 == (tmp = StringUtil::find(tmp, *tmp)))
// 			{
// 				ATLASSERT(TEXT("unclosed quote block : %s"), tmp);
// 				return 0;
// 			}
// 			++tmp;
// 		}
// 
// 		if(match_end(*tmp))
// 			return tmp;
// 	}
// 
// 	return tmp;
// }

bool CommandLineParams::from_string( const tchar* cmd_line)
{
	for (; StringUtil::skip_blank(cmd_line), *cmd_line; )
	{
		switch(*cmd_line)
		{
		case '/':
		case '-':
			if(!parse_named_param(++cmd_line))
				return false;
			break;
		default:
			if(!parse_uname_param(cmd_line))
				return false;
			break;
		}
	}
	return true;
}

bool CommandLineParams::parse_named_param( const tchar* &head )
{
	tstring key = parse_param_block(head, is_parakey_end, true);
	if(key.size() <= 0)
	{
		return false;
	}

	StringUtil::skip_blank(head);
	
	if('=' != *head && ':' != *head)		// �жϲ����Ƿ���ֵ
	{
		set_named_param(key.c_str(), 0);
		return true;
	}

	++head;
	tstring val = parse_param_block(head, StringUtil::is_blank_char<tchar>, true);

	if (val.size() <= 0)
	{
		ATLASSERT(TEXT("unexpected named param k|v split char!"));
	}

	set_named_param(key.c_str(), val.c_str());
	
	return true;
}

bool CommandLineParams::parse_uname_param( const tchar* &head )
{
	tstring result = parse_param_block(head, StringUtil::is_blank_char<tchar>, false);
	if (result.size() > 0)
	{
		append_unnamed_param(result.c_str());
	}
	return true;
}

tstring CommandLineParams::parse_param_block( const tchar* &head, bool(*match_end)(tchar), bool head_blank )
{
	tstring result;
	if(head_blank)								// ȥ����ʼ���Ŀո�
	{
		StringUtil::skip_blank(head);
	}
	
	if(match_end(*head))
	{
		ATLTRACE2(TEXT("unexpected null block!"));
		ATLASSERT(FALSE);
	}

	const tchar* tmp = head;
	for (; *tmp; ++tmp)	// �����ַ���ֱ������������ַ��������е��ַ����⣩
	{
		if('"' == *tmp || '\'' == *tmp)
		{
			if(0 == (tmp = _tcschr(tmp + 1, *tmp)))
			{
				head = tmp + _tcslen(tmp);
				ATLTRACE2(TEXT("unclosed quote block : %s"), tmp);
				ATLASSERT(FALSE);
				return result;
			}
			continue;
		}

		if(match_end(*tmp))
			break;
	}

	if((*head == '"' || *head == '\'') && tmp[-1] == *head)
	{
		tstring(head + 1, tmp - 1).swap(result);
	}
	else
	{
		tstring(head, tmp).swap(result);
	}
	head = tmp;
	return result;
}

bool CommandLineParams::shall_quote_block( const tchar* block, char& usabled_quote)
{
	bool result = false;
	bool single_quote_usable = true, double_quote_usable = true;

	//1. ����ֵ�Կո�ð�š��ȺŴ�ͷ���򳤶�Ϊ��ʱ����Ҫ����
	const tchar* p = block;
	StringUtil::skip_blank(p);
	if(p != block || ':' == *p || '=' == *p || '/' == *p || '-' == *p || 0 == *block)	
		result = true;

	//2. �������
	for (; *p; ++p)
	{
		if(StringUtil::is_blank_char(*p))
		{
			result = true;
		}
		else if('"' == *p || '\'' == *p)
		{
			if('"' == *p)
				double_quote_usable = false;
			else
				single_quote_usable = false;

			const tchar* tmp  = _tcschr(p + 1, *p);		// �����Ƿ�����Ե�����
			if(0 != tmp)								//   ���ҵ����򲻼������������
				p = tmp;								
			else										//   ������ֻ�������֣���Ϊ�������ʱ����������߱���������һ������
				result = true;
		}
	}

	//3. �����飬������ֵ�Կո�ð�š��ȺŽ���ʱ����Ҫ����
	if(!result)
	{
		if(StringUtil::is_blank_char(p[-1]) || '=' == p[-1] || ':' == p[-1])
			result = true;
	}

	//4. ������ͷβ����ԵĴºţ���Ϊ�˽���ʱ��������ȥ������Ҫ�ټ�һ������
	if(!result)
	{
		if(('\'' == *block || '"' == *block) && *block == p[-1])
		{
			result =true;
		}
	}

	if(result)
	{
		if(double_quote_usable)
			usabled_quote = '"';
		else if(single_quote_usable)
			usabled_quote = '\'';
		else
			usabled_quote = 0;
	}

	return result;
}

bool CommandLineParams::to_string(tstring& buf)
{
	bool result = true;
	
	for(NamedParam* p = first_named_param(); p; p->next())
	{
		char quote_char_name, quote_char_val;
		bool quote_need_name, quote_need_val;
		quote_need_name = shall_quote_block(p->name().c_str(), quote_char_name);
		quote_need_val = shall_quote_block(p->val().c_str(), quote_char_val);

		if(quote_need_name && 0 == quote_char_name || quote_need_val && 0 == quote_char_val)
		{
			ATLASSERT(result = false); // �����ַ�����ͬʱ������˫���ţ��޷�����
			continue;
		}

		buf.append(TEXT(" /"));
		if(quote_need_name) buf.append(1, quote_char_name);
		buf.append(p->name());
		if(quote_need_name) buf.append(1, quote_char_name);

		if(p->val().size() > 0)
		{
			buf.append(1, '=');
			if(quote_need_val) buf.append(1, quote_char_val);
			buf.append(p->val());
			if(quote_need_val) buf.append(1, quote_char_val);
		}
	}

	for (UnnamedParam* p = first_unnamed_param(); p; p = p->next())
	{
		buf.append(1, ' ');
		char quote_char_val;
		bool quote_need_val;

		quote_need_val = shall_quote_block(p->val().c_str(), quote_char_val);

		if(quote_need_val && 0 == quote_char_val)	// �����ַ�����ͬʱ������˫���ţ��޷�����
		{
			ATLASSERT(result = false); 
			continue;
		}

		if(quote_need_val) buf.append(1, quote_char_val);
		buf.append(p->val().c_str());
		if(quote_need_val) buf.append(1, quote_char_val);
	}

	return result;
}

CommandLineParams::NamedParam::NamedParam( const tchar* name )
{
	if(name)
		name_.assign(name);
}

CommandLineParams::UnnamedParam* CommandLineParams::first_unnamed_param()
{
	return unnamed_param_head_.next();
}

CommandLineParams::UnnamedParam* CommandLineParams::find_unnamed_param( const tchar* val )
{
	if(0 == val || 0 == *val)
		return 0;

	for (UnnamedParam* param = first_unnamed_param(); param; param = param->next())
	{
		if (0 == _tcscmp(param->val().c_str(), val))
		{
			return param;
		}
	}
	return 0;
}

void CommandLineParams::append_unnamed_param( const tchar* val )
{
	if (val && *val)
	{
		if(!find_unnamed_param(val))
		{
			UnnamedParam* param = new UnnamedParam;
			param->set_val(val);

			param->next_ = unnamed_param_head_.next_;
			unnamed_param_head_.next_ = param;
		}
	}
}

void CommandLineParams::remove_unnamed_param( const tchar* val )
{
	if(0 == val || 0 == *val)
		return;

	for (UnnamedParam* param = &unnamed_param_head_; param; param = param->next())
	{
		UnnamedParam* next_param = param->next();
		if (next_param && 0 == _tcscmp(next_param->val().c_str(), val))
		{
			param->next_ = next_param->next();
			delete param;
			break;
		}
	}
}



CommandLineParams::NamedParam* CommandLineParams::first_named_param()
{
	return (NamedParam*)named_param_head_.next();
}

CommandLineParams::NamedParam* CommandLineParams::find_named_param( const tchar* name )
{
	if(0 == name || 0 == *name)
	{
		return 0;
	}

	for (NamedParam* param = first_named_param(); param; param = param->next())
	{
		if (0 == _tcscmp(param->name().c_str(), name))
		{
			return param;
		}
	}
	return 0;
}

CommandLineParams& CommandLineParams::set_named_param( const tchar* name, const tchar* val )
{
	if(name && *name)
	{
		NamedParam* param = find_named_param(name);
		if(0 == param)
		{
			param = new NamedParam(name);

			param->next_ = named_param_head_.next();
			named_param_head_.next_ = param;
		}
		param->set_val(val);
	}

	return *this;
}

void CommandLineParams::remove_named_param( const tchar* name )
{
	if(0 == name || 0 == *name)
		return;

	for (NamedParam* param = &named_param_head_; param; param = param->next())
	{
		NamedParam* next_param = param->next();
		if (next_param && 0 == _tcscmp(next_param->name().c_str(), name))
		{
			param->next_ = next_param->next();
			delete param;
			break;
		}
	}
}

CommandLineParams::CommandLineParams()
{
	named_param_head_.next_ = 0;
	unnamed_param_head_.next_ = 0;
}

void CommandLineParams::clear()
{
	for (NamedParam* param = first_named_param(); param; )
	{
		NamedParam* tmp = param;
		param = param->next();
		delete tmp;
	}

	for (UnnamedParam* param = first_unnamed_param(); param; )
	{
		UnnamedParam* tmp = param;
		param = param->next();
		delete tmp;
	}
}

CommandLineParams::~CommandLineParams()
{
	clear();
}

bool CommandLineParams::NamedParam::set_val( const tchar* val )
{
	if(val)
		value_.assign(val);
	else
		value_.clear();

	return true;
}

bool CommandLineParams::UnnamedParam::set_val( const tchar* val )
{
	if(0 == val || 0 == *val)
	{
		ATLASSERT(FALSE);
		return false;
	}
	else
	{
		value_.assign(val);
	}
	return true;
}