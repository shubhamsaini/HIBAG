\name{hlaCompareAllele}
\alias{hlaCompareAllele}
\title{
    Evaluate prediction accuracies
}
\description{
    To evaluate the overall accuracy, sensitivity, specificity, positive
predictive value, negative predictive value.
}
\usage{
hlaCompareAllele(TrueHLA, PredHLA, allele.limit=NULL, call.threshold=NaN,
    match.threshold=NaN, max.resolution="", output.individual=FALSE,
    verbose=TRUE)
}
\arguments{
    \item{TrueHLA}{an object of \code{\link{hlaAlleleClass}}, the true
        HLA types}
    \item{PredHLA}{an object of \code{\link{hlaAlleleClass}}, the predicted
        HLA types}
    \item{allele.limit}{a list of HLA alleles, the validation samples are
        limited to those having HLA alleles in \code{allele.limit}, or
        \code{NULL} for no limit. \code{allele.limit} could be character-type,
        \code{\link{hlaAttrBagClass}} or \code{\link{hlaAttrBagObj}}}
    \item{call.threshold}{the call threshold for posterior probability, i.e.,
        call or no call is determined by whether \code{prob >= call.threshold}
        or not}
    \item{match.threshold}{the matching threshold for SNP haplotype similiarity,
        e.g., use 1\% quantile of matching statistics of a training model}
    \item{max.resolution}{"2-digit", "4-digit", "6-digit", "8-digit", "allele",
        "protein", "2", "4", "6", "8", "full" or "": "allele" = "2-digit",
        "protein" = "4-digit", "full" and "" indicating no limit on resolution}
    \item{output.individual}{if TRUE, output accuracy for each individual}
    \item{verbose}{if TRUE, show information}
}
\value{
    Return a \code{list(overall, confusion, detail)}, or
\code{list(overall, confusion, detail, individual)} if
\code{output.individual=TRUE}.

    \code{overall} (data.frame):
    \item{total.num.ind}{the total number of individuals}
    \item{crt.num.ind}{the number of individuals with correct HLA types}
    \item{crt.num.haplo}{the number of chromosomes with correct HLA alleles}
    \item{acc.ind}{the proportion of individuals with correctly predicted
        HLA types (i.e., both of alleles are correct, the accuracy of an
        individual is 0 or 1.)}
    \item{acc.haplo}{the proportion of chromosomes with correctly predicted
        HLA alleles (i.e., the accuracy of an individual is 0, 0.5 or 1,
        since an individual has two alleles.)}
    \item{call.threshold}{call threshold, if it is \code{NaN}, no call
        threshold is executed}
    \item{n.call}{the number of individuals with call}
    \item{call.rate}{overall call rate}

    \code{confusion} (matrix): a confusion matrix.

    \code{detail} (data.frame):
    \item{allele}{HLA alleles}
    \item{train.num}{the number of training haplotypes}
    \item{train.freq}{the training haplotype frequencies}
    \item{valid.num}{the number of validation haplotypes}
    \item{valid.freq}{the validation haplotype frequencies}
    \item{call.rate}{the call rates for HLA alleles}
    \item{accuracy}{allele accuracy}
    \item{sensitivity}{sensitivity}
    \item{specificity}{specificity}
    \item{ppv}{positive predictive value}
    \item{npv}{negative predictive value}
    \item{miscall}{the most likely miss-called alleles}
    \item{miscall.prop}{the proportions of the most likely miss-called
        allele in all miss-called alleles}

    \code{individual} (data.frame):
    \item{sample.id}{sample id}
    \item{true.hla}{the true HLA type}
    \item{pred.hla}{the prediction of HLA type}
    \item{accuracy}{accuracy, 0, 0.5, or 1}
}
\author{Xiuwen Zheng}
\seealso{
    \code{\link{hlaAttrBagging}}, \code{\link{predict.hlaAttrBagClass}},
    \code{\link{hlaReport}}
}

\examples{
# make a "hlaAlleleClass" object
hla.id <- "A"
hla <- hlaAllele(HLA_Type_Table$sample.id,
    H1 = HLA_Type_Table[, paste(hla.id, ".1", sep="")],
    H2 = HLA_Type_Table[, paste(hla.id, ".2", sep="")],
    locus=hla.id, assembly="hg19")

# divide HLA types randomly
set.seed(100)
hlatab <- hlaSplitAllele(hla, train.prop=0.5)
names(hlatab)
# "training"   "validation"
summary(hlatab$training)
summary(hlatab$validation)

# SNP predictors within the flanking region on each side
region <- 500   # kb
snpid <- hlaFlankingSNP(HapMap_CEU_Geno$snp.id, HapMap_CEU_Geno$snp.position,
    hla.id, region*1000, assembly="hg19")
length(snpid)  # 275

# training and validation genotypes
train.geno <- hlaGenoSubset(HapMap_CEU_Geno,
    snp.sel=match(snpid, HapMap_CEU_Geno$snp.id),
    samp.sel=match(hlatab$training$value$sample.id,
    HapMap_CEU_Geno$sample.id))
test.geno <- hlaGenoSubset(HapMap_CEU_Geno,
    samp.sel=match(hlatab$validation$value$sample.id,
    HapMap_CEU_Geno$sample.id))

# train a HIBAG model
set.seed(100)
model <- hlaAttrBagging(hlatab$training, train.geno, nclassifier=4,
    verbose.detail=TRUE)
summary(model)

# validation
pred <- predict(model, test.geno)
# compare
(comp <- hlaCompareAllele(hlatab$validation, pred, allele.limit=model,
    call.threshold=0))
(comp <- hlaCompareAllele(hlatab$validation, pred, allele.limit=model,
    call.threshold=0.5))
}

\keyword{HLA}
\keyword{genetics}
