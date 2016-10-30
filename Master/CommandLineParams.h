/********************************************************************
	Created :	2012-11-22   11:20
	FileName:	CommandLineParams.h
	Author:		Yedaoquan
	Purpose:	
*********************************************************************/
#ifndef _CommandLineParams_h__
#define _CommandLineParams_h__

#include "tstring.h"
#include <vector>

class CommandLineParams
{
public:
	class UnnamedParam
	{
		friend class CommandLineParams;
	protected:
		UnnamedParam*		next_;
		tstring				value_;

	protected:
		UnnamedParam() {}
	
	public:
		UnnamedParam*		next() { return next_; }
		const UnnamedParam*	next() const { return next(); }

		const tstring&		val() const { return value_; }
		bool				set_val(const tchar* val);
	};

	class NamedParam
	{
		friend class CommandLineParams;
	protected:
		NamedParam*			next_;
		tstring				value_;
		tstring				name_;

	protected:
		NamedParam(const tchar* name);
		NamedParam() {}

	public:
		NamedParam*			next() { return next_; }
		const NamedParam*	next() const { return next(); }

		const tstring&		val() const { return value_; }
		bool				set_val(const tchar* val);

		const tstring&		name() const{ return name_; }
	};

public:
	CommandLineParams();
	~CommandLineParams();

	UnnamedParam*	first_unnamed_param();
	UnnamedParam*	find_unnamed_param(const tchar* val);
	void			append_unnamed_param(const tchar* val);
	void			remove_unnamed_param(const tchar* val);

	NamedParam*		first_named_param();
	NamedParam*		find_named_param(const tchar* name);
	CommandLineParams&	set_named_param(const tchar* name, const tchar* val);
	void			remove_named_param(const tchar* name);

	void			clear();

protected:
	UnnamedParam	unnamed_param_head_;
	NamedParam		named_param_head_;

public:

	bool			from_string(const tchar* cmd_line);
	bool			to_string(tstring& buf);

protected:
	bool			parse_named_param(const tchar* &head);
	bool			parse_uname_param(const tchar* &head);
	tstring			parse_param_block(const tchar* &head, bool(*match_end)(tchar), bool head_blank);
	bool			shall_quote_block(const tchar* block, char& usabled_quote);
};

#endif // _CommandLineParams_h__
