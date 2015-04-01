module DHT11_Drive
(
	input CLK, RST,
	inout Pin_Data,
	
	output [7:0] RH_Data,
	output [7:0] T_Data,
	output [7:0] Com_Data
);

	parameter T10us = 12'd199;
	parameter T30us = 12'd599;
	parameter T40us = 12'd799;
	parameter T78us = 12'd1559;
	parameter T80us = 12'd1599;
	parameter T120us = 12'd2399;
	parameter T140us = 12'd2799;
	parameter T18ms = 20'd359999;
/************计数器***********/	
	reg [19:0] cnt;
	reg cnt_com;
	always@( posedge CLK or negedge RST )
	begin
		if( !RST )
			cnt <= 20'd0;
		else if( cnt_com )
			cnt <= cnt + 1'b1;
		else if( !cnt_com )
			cnt <= 20'd0;
	end
	
	reg [5:0] i;
	reg rACK;
	reg [11:0] C1;
//	reg [7:0] rRH_Data;
//	reg [7:0] rT_Data;
//	reg [7:0] rCom_Data;
	reg [39:0] temp;
	reg rPin_Data;
	reg Out;
	always@( posedge CLK or negedge RST )
	begin
		if( !RST )
		begin
	//		rRH_Data <= 8'd0;
	//		rT_Data <= 8'd0;
	//		rCom_Data <= 8'd0;
			temp <= 40'd0;
			i <= 6'd0;
			C1 <= 12'd0;
			rACK <= 1'b1;
			rPin_Data <= 1'b1;
			Out <= 1'b1;
		end
		else case( i )
			0:             //主机发送开始信号
			begin
				Out = 1'b1;
				if( cnt == T18ms )
				begin
					rPin_Data <= 1'b1;
					cnt_com <= 1'b0;
					i <= i + 1'b1;
				end
				else
				begin
					rPin_Data <= 1'b0;
					cnt_com <= 1'b1;
				end
			end
			1:                  //延时40us
			begin
				Out = 1'b1;
				if( cnt == T40us )
				begin
					rPin_Data <= 1'b1;
					cnt_com <= 1'b0;
					i <= i + 1'b1;
				end
				else
					cnt_com <= 1'b1;
			end
			2:
			begin
				Out = 1'b0;           //从机应答
				if( C1 == T40us )
					rACK <= Pin_Data;
				if( C1 == T80us )
				begin
					C1 <= 12'd0;
					i <= i + 1'b1;
				end
				else
					C1 <= C1 + 1'b1;
			end
			3:
				if( rACK != 0 )
					i <= 6'd0;
				else 
					i <= i + 1'b1;
			4:                      //拉高，准备输出
			begin
				Out = 1'b0;
				if( C1 == T40us )
					rACK <= Pin_Data;
				if( C1 == T80us )
				begin
					C1 <= 12'd0;
					i <= i + 1'b1;
				end
				else
					C1 <= C1 + 1'b1;
			end
			5:
				if( rACK == 0 )
					i <= 6'd0;
				else 
					i <= i + 1'b1;
					
			6 ,7 ,8 ,9 ,10,11,12,13,
			14,15,16,17,18,19,20,21,
			22,23,24,25,26,27,28,29,
			30,31,32,33,34,35,36,37,
			38,39,40,41,42,43,44,45:
			begin
				Out = 1'b0;
				if( C1 == T78us )
				begin
					if( Pin_Data )
						temp <= { temp[39:0] + 1'b1 };
					else 
					begin
						temp <= { temp[39:0] + 1'b0 };
						C1 <= 12'd0;
						i <= i + 1'b1;
					end
				end
				if( C1 == T120us )
				begin
					C1 <= 12'd0;
					i <= i + 1'b1;
				end
				else
					C1 <= C1 + 1'b1;	
			end	
			46:
			begin
				Out = 1'b0;
				if( C1 == T140us )
				begin
					rACK <= Pin_Data;
					C1 <= 12'd0;
					i <= i + 1'b1;
				end
				else
					C1 <= C1 + 1'b1;
			end
			47:
				if( !rACK )
					i <= 6'd0;
				else
					i <= 6'd0;
		endcase
	end
	
	assign Pin_Data = Out? rPin_Data : 1'bz;
	assign RH_Data = temp[39:32];
	assign T_Data = temp[23:16];
	assign Com_Data = temp[7:0];

endmodule
