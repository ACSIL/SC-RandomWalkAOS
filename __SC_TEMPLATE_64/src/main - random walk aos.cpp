#include "sierrachart.h"
#include<random>
#include<chrono>
#include<string>

SCDLLName("RANDOM WALK AOS2107")

constexpr int seconds_per_day{ 86400 };

unsigned int rn_generator(SCStudyInterfaceRef sc)
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(sc.Input[10].GetInt(), sc.Input[11].GetInt()); //set min do max to define trading frequency - the higher the max number, the less entries within a day
	return distribution(gen);
}

unsigned char ls_generator()
{
	std::mt19937 gen;
	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	std::uniform_int_distribution<int> distribution(0, 1); //generate 1/0 to decide for long/short
	return distribution(gen);
}

bool is_rth(SCStudyInterfaceRef sc)
{
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > sc.Input[1].GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < sc.Input[2].GetTime())
		return 1;
	else
		return 0;
}

SCSFExport scsf_random_walk_aos(SCStudyInterfaceRef sc)
{
	SCInputRef display_values = sc.Input[0];
	SCInputRef rth_start = sc.Input[1];
	SCInputRef rth_finish = sc.Input[2];
	SCInputRef flat_time = sc.Input[3];
	SCInputRef trade_management = sc.Input[4];
	SCInputRef ATR_multiplier = sc.Input[5];

	if (sc.SetDefaults)
	{

		display_values.Name = "Display Values";
		display_values.SetYesNo(1);

		rth_start.Name = "Trade From:";
		rth_start.SetTime(HMS_TIME(8, 30, 0));
		rth_finish.Name = "Trade Till:";
		rth_finish.SetTime(HMS_TIME(15, 00, 00));
		flat_time.Name = "Flat At:";
		flat_time.SetTime(HMS_TIME(15, 10, 00));

		trade_management.Name = "Trade management";
		trade_management.SetCustomInputStrings("Fixed; ATR Based;");
		trade_management.SetCustomInputIndex(0);

		ATR_multiplier.Name = "ATR Multiplier";
		ATR_multiplier.SetFloat(5.0f);
		
		sc.Input[10].Name = "Random numbers range - min";
		sc.Input[10].SetInt(10);
		sc.Input[11].Name = "Random numbers range - max";
		sc.Input[11].SetInt(3600);

		sc.GraphName = "Randow walk AOS 2017";
		sc.AutoLoop = 1;
		sc.GraphRegion = 0;
		sc.UpdateAlways = 1;
		sc.FreeDLL = 1;

		return;
	}

	sc.AllowMultipleEntriesInSameDirection = true;
	sc.MaximumPositionAllowed = 1000;
	sc.SupportReversals = false;
	sc.SendOrdersToTradeService = false;
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
	sc.SupportAttachedOrdersForTrading = true;
	sc.CancelAllOrdersOnEntriesAndReversals = false;
	sc.AllowEntryWithWorkingOrders = false;
	sc.CancelAllWorkingOrdersOnExit = false;
	sc.AllowOnlyOneTradePerBar = true;
	sc.MaintainTradeStatisticsAndTradesData = true; //has to be set

	//control perzist vars (cpv)
	int &cpv_already_traded = sc.GetPersistentInt(100);
	int &cpv_already_generated = sc.GetPersistentInt(101);
	int &cpv_previous_qty = sc.GetPersistentInt(102);

	//generates the random numbers
	int &x = sc.GetPersistentInt(1);
	int &ls = sc.GetPersistentInt(3);
	if (!sc.IsFullRecalculation && !cpv_already_generated)
	{
		x = rn_generator(sc);
		ls = ls_generator();
		cpv_already_generated = 1;
	}
	const char * dir = (ls == 1) ? "Long" : "Short";

	//set the timestamps
	int curent_time = sc.BaseDateTimeIn[sc.Index].GetTime();
	SCDateTime &last_exit_dt_perzist = sc.GetPersistentSCDateTime(2);
	int exit_hour, exit_minute, exit_second;
	last_exit_dt_perzist.GetTimeHMS(exit_hour, exit_minute, exit_second);
	int last_exit_time = last_exit_dt_perzist.GetTime();
	int starting_time = rth_start.GetTime(); // at first set the starting point to start RTH
	if (cpv_already_traded != 0) { starting_time = last_exit_time; } // after each entry reset it to the timestamp of the last exit
	int time_to_entry = x + starting_time;
	
	//set orders
	s_SCNewOrder regular_order;
	regular_order.OrderType = SCT_ORDERTYPE_MARKET;
	regular_order.OrderQuantity = 1;

	s_SCNewOrder ATR_order;
	ATR_order.OrderType = SCT_ORDERTYPE_MARKET;
	ATR_order.OrderQuantity = 1;

	//compute ATR based sl and pt
	SCSubgraphRef sg_ATR = sc.Subgraph[0];
	sc.ATR(sc.BaseDataIn, sg_ATR, 10, MOVAVGTYPE_SIMPLE);
	float ATR_value = sg_ATR[sc.Index];
	ATR_order.Target1Offset = ATR_value * ATR_multiplier.GetFloat() * sc.TickSize;
	ATR_order.Stop1Offset = ATR_value * ATR_multiplier.GetFloat() * sc.TickSize;
		
	//set position
	s_SCPositionData current_position;
	sc.GetTradePosition(current_position);
	int position_qty{ static_cast<int>(current_position.PositionQuantity) };

	if (is_rth(sc))
	{
		if (curent_time >= time_to_entry && ls == 1)
		{
			switch (trade_management.GetIndex())
			{
				case 0: sc.BuyEntry(regular_order);
					break;
				case 1: sc.BuyEntry(ATR_order);
					break;
			}
			if (position_qty != 0)
			{
				cpv_previous_qty = 1; 
				x = seconds_per_day; //to avoid reentry in case the trade would take really long (no trade till take longer than one day)
			}
		}
		else if (curent_time >= time_to_entry && ls == 0)
		{
			switch (trade_management.GetIndex())
			{
				case 0:	sc.BuyEntry(regular_order);
					break;
				case 1:	sc.BuyEntry(ATR_order);
					break;
			}
			if (position_qty != 0)
			{
				cpv_previous_qty = 1;
				x = seconds_per_day;
			}
		}
		// after closing a possition
		if (cpv_previous_qty != 0 && position_qty == 0) 
		{
				last_exit_dt_perzist = current_position.LastExitDateTime;  
				cpv_already_traded = 1;
				cpv_previous_qty = 0;
				cpv_already_generated = 0; //reset the control perzist var to re-generate random number
		}
	}
	
	if (current_position.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= flat_time.GetTime()) sc.FlattenAndCancelAllOrders();

	if (curent_time == 0) sc.ClearAllPersistentData(); 

	if (display_values.GetYesNo() == TRUE)
	{
		{
			s_UseTool atr;
			atr.Clear();
			atr.ChartNumber = sc.ChartNumber;
			atr.DrawingType = DRAWING_TEXT;
			atr.FontSize = 10;
			atr.FontBold = false;
			atr.AddMethod = UTAM_ADD_OR_ADJUST;
			atr.UseRelativeVerticalValues = 1;
			atr.BeginDateTime = 4;
			atr.BeginValue = 87;
			atr.Color = RGB(255, 255, 255);
			atr.Region = 0;
			atr.Text.Format("ATR: %3.2f", ATR_value);
			atr.LineNumber = 22;
			sc.UseTool(atr);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 99;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Random Number: %i", x);
			t.LineNumber = 2;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 97;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Starting Time as int: %i", starting_time);
			t.LineNumber = 5;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 95;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Next Entry Time as int: %i", time_to_entry);
			t.LineNumber = 3;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 93;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Current Time as int: %i", curent_time);
			t.LineNumber = 1;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 91;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Next Entry Direction: %s", dir);
			t.LineNumber = 4;
			sc.UseTool(t);
		} {
			s_UseTool t;
			t.Clear();
			t.ChartNumber = sc.ChartNumber;
			t.DrawingType = DRAWING_TEXT;
			t.FontSize = 10;
			t.FontBold = false;
			t.AddMethod = UTAM_ADD_OR_ADJUST;
			t.UseRelativeVerticalValues = 1;
			t.BeginDateTime = 4;
			t.BeginValue = 89;
			t.Color = RGB(255, 255, 255);
			t.Region = 0;
			t.Text.Format("Last Exit Time as int: %i", last_exit_time);
			t.LineNumber = 8;
			sc.UseTool(t);
		}
	}
}
