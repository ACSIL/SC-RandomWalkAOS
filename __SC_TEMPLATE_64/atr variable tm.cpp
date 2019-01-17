

//po mym kliknuti posle pt a sl s oco vazbou ve vzdalenosti podle aktualni volatily s rr 1:1
SCSFExport scsf_VariableTradeManagement(SCStudyInterfaceRef sc)
{
	SCInputRef multiplicator = sc.Input[0];

	if (sc.SetDefaults)
	{
		sc.Input[0].Name = "Multiplicator";
		sc.Input[0].SetFloat(3.0f);
		sc.GraphName = "Variable Trade Management";
		sc.AutoLoop = 1;
		sc.GraphRegion = 0;
		sc.FreeDLL = 1;
		return;
	}

	sc.SupportAttachedOrdersForTrading = true;
	sc.AllowOnlyOneTradePerBar = false;
	sc.MaximumPositionAllowed = 100;

	s_SCPositionData current_position;
	sc.GetTradePosition(current_position);

	//calculate atr value
	sc.ATR(sc.BaseDataIn, sc.Subgraph[0], 10, MOVAVGTYPE_SIMPLE);
	float atr_value = sc.Subgraph[0][sc.Index];
	{
		s_UseTool atr;
		atr.Clear();
		atr.ChartNumber = sc.ChartNumber;
		atr.DrawingType = DRAWING_TEXT;
		atr.FontSize = 10;
		atr.FontBold = false;
		atr.AddMethod = UTAM_ADD_OR_ADJUST;
		atr.UseRelativeVerticalValues = 1;
		atr.BeginDateTime = 7;
		atr.BeginValue = 87;
		atr.Color = RGB(255, 255, 255);
		atr.Region = 0;
		atr.Text.Format("ATR: %3.2f", atr_value);
		atr.LineNumber = 2;
		sc.UseTool(atr);
	}

	//define exit prices for long
	double long_SL = current_position.AveragePrice - multiplicator.GetFloat() * atr_value / sc.TickSize;
	double long_PT = current_position.AveragePrice + multiplicator.GetFloat() * atr_value / sc.TickSize;
	s_SCNewOrder long_exit;
	long_exit.OrderType = SCT_ORDERTYPE_OCO_LIMIT_STOP;
	long_exit.Price1 = long_PT;
	long_exit.Price2 = long_SL;

	//define exit prices for short
	double short_SL = current_position.AveragePrice + multiplicator.GetFloat() * atr_value / sc.TickSize;
	double short_PT = current_position.AveragePrice - multiplicator.GetFloat() * atr_value / sc.TickSize;
	s_SCNewOrder short_exit;
	short_exit.OrderType = SCT_ORDERTYPE_OCO_LIMIT_STOP;
	short_exit.Price1 = short_PT;
	short_exit.Price2 = short_SL;

	//set sl and pt after each entry
	int &previous_qt_perzist = sc.GetPersistentInt(0);
	t_OrderQuantity32_64 qty{ current_position.PositionQuantity };
	if (previous_qt_perzist == 0 && current_position.PositionQuantity > 0) // from no possition to long possition
	{
		t_OrderQuantity32_64 succesful_entry = sc.BuyExit(long_exit);
		if (succesful_entry > 0)
		{
			previous_qt_perzist = static_cast<int>(current_position.PositionQuantity);
		}
	}
	else if (previous_qt_perzist == 0 && current_position.PositionQuantity < 0) // from no possition to short possition
	{
		t_OrderQuantity32_64 succesful_entry = sc.SellExit(short_exit);
		if (succesful_entry > 0)
		{
			previous_qt_perzist = static_cast<int>(current_position.PositionQuantity);
		}
	}
	//reset the perzist
	if (current_position.PositionQuantity == 0) previous_qt_perzist = 0;
}


