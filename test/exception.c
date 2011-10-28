#include "Config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "assert.h"
#include "Thread.h"
#include "Exception.h"
#include "SQLException.h"
#include "AssertException.h"


/**
 * Exceptions handling unit tests
 */

#define THREADS 50

Exception_T A = {"AException"};
Exception_T B = {"BException"};
Exception_T C = {"CException"};
Exception_T D = {"DException"};

void throwA() {
        THROW(A, "A");
}

void throwB() {
        THROW(B, "B");
}

void throwC() {
        THROW(C, "C");
}

void throwD() {
        THROW(D, "D");
}

void indirectA() {
        throwA();
}

/* Throw and catch exceptions and check that we get the expected exception. 
 * If the exception stack is corrupt this should be detected
 */
void *thread(void *args) {
        TRY
                THROW(A, "A");
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(D, "D");
                assert(false); // Should not be reached
        CATCH(D)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(C, "C");
                assert(false); // Should not be reached
        CATCH(C)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(A, "A");
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                throwA();
                throwB();
                throwC();
                throwD();
        CATCH(A)
                // Ok
        CATCH(B)
                assert(false); // Should not be reached
        CATCH(C)
                assert(false); // Should not be reached
        CATCH(D)
                assert(false); // Should not be reached
        END_TRY;
        TRY
                indirectA();
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(A, "A");
                assert(false); // Should not be reached
        CATCH(A)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(B, "B");
                assert(false); // Should not be reached
        CATCH(B)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                THROW(D, "D");
                assert(false); // Should not be reached
        CATCH(D)
                // Ok
        ELSE
                assert(false); // Should not be reached
        END_TRY;
        TRY
                RETURN (NULL);
                assert(false); // Should not be reached
        CATCH(A)
                assert(false); // Should not be reached
        END_TRY;
        return NULL; 
}


int main(void) {
        Exception_init();
        printf("============> Start Exeption Tests\n\n");
        
        printf("=> Test1: TRY-CATCH\n");
        {
                TRY
                        THROW(A, NULL);
                        assert(false); // Should not be reached
                CATCH(A)
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test1: OK\n\n");
        
        printf("=> Test2: TRY-CATCH indirect throw\n");
        {
                TRY
                        indirectA();
                        assert(false); // Should not be reached
                CATCH(A)
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test2: OK\n\n");
        
        printf("=> Test3: TRY-ELSE\n");
        {
                TRY
                        THROW(B, "B");
                        assert(false); // Should not be reached
                ELSE
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test3: OK\n\n");
 
        printf("=> Test4: TRY-CATCH-ELSE\n");
        {
                TRY
                        throwB();
                        assert(false); // Should not be reached
                CATCH(A)
                        assert(false); // Should not be reached
                ELSE
                        printf("\tResult: Ok\n");
                END_TRY;
        }
        printf("=> Test4: OK\n\n");

        printf("=> Test5: TRY-FINALLY\n");
        {
                int i = 0;
                TRY
                        i++;
                FINALLY
                        printf("\tResult: ok\n");
                        assert(i==1);
                END_TRY;
        }
        printf("=> Test5: OK\n\n");

        printf("=> Test6: TRY-CATCH-FINALLY and volatile\n");
        {
                volatile int i = 0;
                TRY
                        i++;
                        THROW(C, "C");
                CATCH(C)
                        assert(i == 1);
                        i++;
                FINALLY
                        printf("\tResult: ok\n");
                        assert(i == 2);
                END_TRY;
        }
        printf("=> Test6: OK\n\n");
        
        printf("=> Test7: CATCH SQLException\n");
        {
                TRY
                        THROW(SQLException, "SQLException");
                        assert(false); // Should not be reached
                CATCH(SQLException)
                        printf("\tResult: ok got SQLException\n");
                END_TRY;
        }
        printf("=> Test7: OK\n\n");
        
        printf("=> Test8: CATCH AssertException\n");
        {
                TRY
                        assert(false); // Throws AssertException
                        printf("Test8 failed\n"); 
                        exit(1);
                CATCH(AssertException)
                        printf("\tResult: ok got AssertException\n");
                END_TRY;
        }
        printf("=> Test8: OK\n\n");
        
        printf("=> Test9: Nested TRY-CATCH\n");
        {
                TRY
                        TRY
                                THROW(B, "B");
                                THROW(A, "A");
                                assert(false); // Should not be reached
                        CATCH(A)
                                assert(false); // Should not be reached
                        END_TRY;
                CATCH(B)
                        printf("\tResult: ok\n");
                END_TRY;
        }
        printf("=> Test9: OK\n\n");
        
        printf("=> Test10: RETHROW\n");
        {
                TRY
                        TRY
                                THROW(A, "A");
                                assert(false); // Should not be reached
                        CATCH(A)
                                RETHROW;
                        END_TRY;
                CATCH(A)
                        printf("\tResult: ok got Exception\n");
                END_TRY;
        }
        printf("=> Test10: OK\n\n");
        
        printf("=> Test11: No exception thrown\n");
        {
                TRY
                        int i = 0; i++;
                ELSE
                        assert(false); // Should not be reached
                END_TRY;
        }
        printf("=> Test11: OK\n\n");
        
        printf("=> Test12: Test thread-safeness\n");
        {
                int i;
                Thread_T threads[THREADS];
                for (i = 0; i < THREADS; i++)
                        Thread_create(threads[i], thread, NULL);
                for (i = 0; i < THREADS; i++)
                        Thread_join(threads[i]);
        }
        printf("=> Test12: OK\n\n");
        
        printf("============> Exception Tests: OK\n\n");

        return 0;
}
