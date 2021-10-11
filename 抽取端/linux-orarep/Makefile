
ORACLE_HOME=/u01/app/oracle/product/11.2.0/db_1

CPPFLAGS = -fno-strict-aliasing -fno-exceptions -fno-rtti -fPIC
DEBUG = -g

CXX = /usr/bin/g++  $(CPPFLAGS) $(DEBUG)


# Targets
all: lgc_main
RM = /bin/rm
RMF = $(RM) -f


lgc_api.o: lgc_api.cpp lgc_api.h
	$(CXX) lgc_api.cpp -c -o lgc_api.o -I$(ORACLE_HOME)/rdbms/public

lgc_ChangeHandler.o: lgc_ChangeHandler.cpp lgc_ChangeHandler.h
	$(CXX) lgc_ChangeHandler.cpp -c -o lgc_ChangeHandler.o -I$(ORACLE_HOME)/rdbms/public

lgc_ChangeInput.o: lgc_ChangeInput.cpp lgc_ChangeInput.h
	$(CXX) lgc_ChangeInput.cpp -c -o lgc_ChangeInput.o -I$(ORACLE_HOME)/rdbms/public

lgc_Change.o: lgc_Change.cpp lgc_Change.h
	$(CXX) lgc_Change.cpp -c -o lgc_Change.o -I$(ORACLE_HOME)/rdbms/public

lgc_OpcodeParser.o: lgc_OpcodeParser.cpp lgc_OpcodeParser.h
	$(CXX) lgc_OpcodeParser.cpp -c -o lgc_OpcodeParser.o -I$(ORACLE_HOME)/rdbms/public

lgc_RecordHandler.o: lgc_RecordHandler.cpp lgc_RecordHandler.h
	$(CXX) lgc_RecordHandler.cpp -c -o lgc_RecordHandler.o -I$(ORACLE_HOME)/rdbms/public

lgc_RedoRecordInput.o: lgc_RedoRecordInput.cpp lgc_RedoRecordInput.h
	$(CXX) lgc_RedoRecordInput.cpp -c -o lgc_RedoRecordInput.o -I$(ORACLE_HOME)/rdbms/public

lgc_RedoRecord.o: lgc_RedoRecord.cpp lgc_RedoRecord.h
	$(CXX) lgc_RedoRecord.cpp -c -o lgc_RedoRecord.o -I$(ORACLE_HOME)/rdbms/public

lgc_RedoFileInput.o: lgc_RedoFileInput.cpp lgc_RedoFileInput.h
	$(CXX) lgc_RedoFileInput.cpp -c -o lgc_RedoFileInput.o -I$(ORACLE_HOME)/rdbms/public

lgc_RedoFile.o: lgc_RedoFile.cpp lgc_RedoFile.h
	$(CXX) lgc_RedoFile.cpp -c -o lgc_RedoFile.o -I$(ORACLE_HOME)/rdbms/public

lgc_TransactionMgr.o: lgc_TransactionMgr.cpp lgc_TransactionMgr.h
	$(CXX) lgc_TransactionMgr.cpp -c -o lgc_TransactionMgr.o -I$(ORACLE_HOME)/rdbms/public


lgc_Transaction.o: lgc_Transaction.cpp lgc_Transaction.h
	$(CXX) lgc_Transaction.cpp -c -o lgc_Transaction.o -I$(ORACLE_HOME)/rdbms/public

lgc_TransactionHandler.o: lgc_TransactionHandler.cpp lgc_TransactionHandler.h
	$(CXX) lgc_TransactionHandler.cpp -c -o lgc_TransactionHandler.o -I$(ORACLE_HOME)/rdbms/public

lgc_DmlRowsOutput.o: lgc_DmlRowsOutput.cpp lgc_DmlRowsOutput.h
	$(CXX) lgc_DmlRowsOutput.cpp -c -o lgc_DmlRowsOutput.o -I$(ORACLE_HOME)/rdbms/public

lgc_DmlRow.o: lgc_DmlRow.cpp lgc_DmlRow.h
	$(CXX) lgc_DmlRow.cpp -c -o lgc_DmlRow.o -I$(ORACLE_HOME)/rdbms/public

lgc_DmlChangeList.o: lgc_DmlChangeList.cpp lgc_DmlChangeList.h
	$(CXX) lgc_DmlChangeList.cpp -c -o lgc_DmlChangeList.o -I$(ORACLE_HOME)/rdbms/public

lgc_MediaFileOutput.o: lgc_MediaFileOutput.cpp lgc_MediaFileOutput.h
	$(CXX) lgc_MediaFileOutput.cpp -c -o lgc_MediaFileOutput.o -I$(ORACLE_HOME)/rdbms/public

lgc_TConsumer.o: lgc_TConsumer.cpp lgc_TConsumer.h
	$(CXX) lgc_TConsumer.cpp -c -o lgc_TConsumer.o -I$(ORACLE_HOME)/rdbms/public

lgc_DmlRowParser.o: lgc_DmlRowParser.cpp lgc_DmlRowParser.h
	$(CXX) lgc_DmlRowParser.cpp -c -o lgc_DmlRowParser.o -I$(ORACLE_HOME)/rdbms/public

lgc_ora_convert.o: lgc_ora_convert.cpp lgc_ora_convert.h
	$(CXX) lgc_ora_convert.cpp -c -o lgc_ora_convert.o -I$(ORACLE_HOME)/rdbms/public

lgc_TOriginalQueue.o: lgc_TOriginalQueue.cpp lgc_TOriginalQueue.h
	$(CXX) lgc_TOriginalQueue.cpp -c -o lgc_TOriginalQueue.o -I$(ORACLE_HOME)/rdbms/public

lgc_TMergedQueue.o: lgc_TMergedQueue.cpp lgc_TMergedQueue.h
	$(CXX) lgc_TMergedQueue.cpp -c -o lgc_TMergedQueue.o -I$(ORACLE_HOME)/rdbms/public

lgc_ExtractThread.o: lgc_ExtractThread.cpp lgc_ExtractThread.h
	$(CXX) lgc_ExtractThread.cpp -c -o lgc_ExtractThread.o -I$(ORACLE_HOME)/rdbms/public

lgc_MergeThread.o: lgc_MergeThread.cpp lgc_MergeThread.h
	$(CXX) lgc_MergeThread.cpp -c -o lgc_MergeThread.o -I$(ORACLE_HOME)/rdbms/public

lgc_ConsumeThread.o: lgc_ConsumeThread.cpp lgc_ConsumeThread.h
	$(CXX) lgc_ConsumeThread.cpp -c -o lgc_ConsumeThread.o -I$(ORACLE_HOME)/rdbms/public


lgc_param.o: lgc_param.cpp lgc_param.h
	$(CXX) lgc_param.cpp -c -o lgc_param.o -I$(ORACLE_HOME)/rdbms/public

lgc_tableMeta.o: lgc_tableMeta.cpp lgc_tableMeta.h
	$(CXX) lgc_tableMeta.cpp -c -o lgc_tableMeta.o -I$(ORACLE_HOME)/rdbms/public

lgc_readRedoFile.o: lgc_readRedoFile.cpp lgc_readRedoFile.h
	$(CXX) $< -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o $@


Unit.o: Unit.cpp Unit.h
	$(CXX) Unit.cpp -c -o Unit.o -I$(ORACLE_HOME)/rdbms/public

BlockPool.o: BlockPool.cpp tw_ASM.h
	$(CXX) BlockPool.cpp -c -o BlockPool.o -I$(ORACLE_HOME)/rdbms/public

ResourcePool.o: ResourcePool.cpp
	$(CXX) ResourcePool.cpp -c -o ResourcePool.o

dlist.o: fd_dlist.cpp
	$(CXX) fd_dlist.cpp -c -o dlist.o

oracleoci.o: oracleoci.cpp oracleoci.h
	$(CXX) oracleoci.cpp -c -o oracleoci.o -I$(ORACLE_HOME)/rdbms/public/

OciQuery.o: OciQuery.cpp OciQuery.h
	$(CXX) OciQuery.cpp -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o OciQuery.o

ChuckSum.o: ChuckSum.cpp
	$(CXX) ChuckSum.cpp -c -o ChuckSum.o

ini.o:	RedologINI.cpp
	$(CXX) RedologINI.cpp -c -o RedologINI.o

tw_api.o: tw_api.cpp tw_api.h
	$(CXX) tw_api.cpp -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o tw_api.o

tw_file.o: tw_file.cpp tw_file.h
	$(CXX) tw_file.cpp -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o tw_file.o

tw_rwDataBlocks.o: tw_rwDataBlocks.cpp tw_rwDataBlocks.h tw_ASM.h tw_file.h
	$(CXX) tw_rwDataBlocks.cpp -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o tw_rwDataBlocks.o

skip_ASM.o:skip_ASM.cpp skip_ASM.h tw_file.h
	$(CXX) skip_ASM.cpp -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o skip_ASM.o

tw_ASM.o: tw_ASM.cpp tw_ASM.h skip_ASM.h
	$(CXX) $< -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -c -o $@


lgc_main:Unit.o BlockPool.o ResourcePool.o dlist.o oracleoci.o OciQuery.o ChuckSum.o ini.o tw_api.o tw_rwDataBlocks.o tw_file.o skip_ASM.o tw_ASM.o lgc_api.o lgc_RedoFileInput.o lgc_RedoFile.o lgc_RedoRecordInput.o lgc_RedoRecord.o lgc_RecordHandler.o lgc_ChangeInput.o lgc_ChangeHandler.o lgc_OpcodeParser.o lgc_Change.o lgc_TransactionMgr.o lgc_Transaction.o lgc_TransactionHandler.o lgc_DmlRowsOutput.o lgc_DmlRow.o lgc_DmlChangeList.o lgc_TConsumer.o lgc_DmlRowParser.o lgc_ora_convert.o lgc_MediaFileOutput.o lgc_TOriginalQueue.o lgc_TMergedQueue.o lgc_ExtractThread.o lgc_MergeThread.o lgc_ConsumeThread.o lgc_param.o lgc_tableMeta.o lgc_readRedoFile.o lgc_main.cpp 
	$(CXX) Unit.o BlockPool.o ResourcePool.o dlist.o oracleoci.o OciQuery.o ChuckSum.o RedologINI.o tw_api.o tw_rwDataBlocks.o tw_file.o skip_ASM.o tw_ASM.o lgc_api.o lgc_RedoFileInput.o lgc_RedoFile.o lgc_RedoRecordInput.o lgc_RedoRecord.o lgc_RecordHandler.o lgc_ChangeInput.o lgc_ChangeHandler.o lgc_OpcodeParser.o lgc_Change.o lgc_TransactionMgr.o lgc_Transaction.o lgc_TransactionHandler.o lgc_DmlRowsOutput.o lgc_DmlRow.o lgc_DmlChangeList.o lgc_TConsumer.o lgc_DmlRowParser.o lgc_ora_convert.o lgc_MediaFileOutput.o lgc_TOriginalQueue.o lgc_TMergedQueue.o lgc_ExtractThread.o lgc_MergeThread.o lgc_ConsumeThread.o lgc_param.o lgc_tableMeta.o lgc_readRedoFile.o lgc_main.cpp -lpthread -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -lclntsh -o lgc_main

clean: 
	$(RMF) *.o *.bak *~ *.intpro *.extpro 1 2 3
	$(RMF) main test testchuck testfile lgc_main
 
