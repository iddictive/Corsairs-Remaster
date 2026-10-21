extern bool Custody_Begin(ref captor);

void Probe(ref NPChar)
{
    if (Custody_Begin(NPChar) == false)
    {
    }
}
